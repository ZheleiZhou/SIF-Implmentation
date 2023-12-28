#pragma once

#include "emp-agmpc/fpremp.h"
#include "emp-agmpc/abitmp.h"
#include "emp-agmpc/netmp.h"
#include "emp-agmpc/flexible_input_output.h"
#include <emp-tool/emp-tool.h>
using namespace emp;
using namespace std;

template<int nP>
class TwoRound_SIF_Bool{ public:
	const static int SSP = 5;//5*8 in fact...
	const block MASK = makeBlock(0x0ULL, 0xFFFFFULL);
	FpreMP<nP>* fpre = nullptr;
	
	// single values
	block* mac[nP+1];
	block* key[nP+1];
	bool* value;


	// triples values
	block * mt_mac[nP+1];
	block * mt_key[nP+1];
	bool * mt_value;

	// wires values
	block * wires_mac[nP+1];
	block * wires_key[nP+1];
	bool * wires;

	BristolFashion * cf;
	NetIOMP<nP> * io;
	int num_ands = 0, num_in;
	int party, total_pre, ssp;
	ThreadPool * pool;

	bool * mask_input;
	bool * mask_and;
	bool * check_value;
	block * check_mac[nP+1];
	block * check_key[nP+1];


	block Delta;
	PRP prp;

    TwoRound_SIF_Bool(NetIOMP<nP> * io[2], ThreadPool * pool, int party, BristolFashion * cf, bool * _delta = nullptr, int ssp = 40) {
		this->party = party;
		this->io = io[0];
		// cf is the circuit
		this->cf = cf;
		this->ssp = ssp;
		this->pool = pool;
		
		for(int i = 0; i < cf->num_gate; ++i) {
			if (cf->gates[4*i+3] == AND_GATE)
				++num_ands;
		}
		num_in = cf->num_input;
		total_pre = num_in + num_ands + 3*ssp;
		fpre = new FpreMP<nP>(io, pool, party, _delta, ssp);
		Delta = fpre->Delta;

		for(int i  = 1; i <= nP; ++i) {
			key[i] = new block[num_in];
			mac[i] = new block[num_in];
			mt_key[i] = new block[num_ands*3];
			mt_mac[i] = new block[num_ands*3];
			wires_key[i] = new block[cf->num_wire];
			wires_mac[i] = new block[cf->num_wire];
			check_mac[i] = new block[num_ands*2];
			check_key[i] = new block[num_ands*2];
		}
		value = new bool[num_in];
		mt_value = new bool[num_ands*3];
		wires = new bool[cf->num_wire];

		mask_input = new bool[num_in];
		mask_and = new bool[num_ands*2];
		check_value = new bool[2*num_ands];
	}
	~TwoRound_SIF_Bool() {
		for(int i = 1; i <= nP; ++i) {
			delete[] key[i];
			delete[] mac[i];
			delete[] mt_key[i];
			delete[] mt_mac[i];
			delete[] wires_key[i];
			delete[] wires_mac[i];
			delete[] check_mac[i];
			delete[] check_key[i];
		}
		delete[] value;
		delete[] mt_value;
		delete[] wires;
		delete[] mask_input;
		delete[] mask_and;
		delete[] check_value;
	}
	PRG prg;

	void Preprocess(){
		// obtain the preprocessing values for AND gates
		if (num_ands != 0)
			fpre->compute(mt_mac, mt_key, mt_value, num_ands);

		// obtain the preprocessing values for input wires
		prg.random_bool(value, num_in);
		fpre->abit->compute(mac, key, value, num_in);
		//auto ret = fpre->abit->check(mac, key, value, num_in);
		//ret.get();

		// for preprocessing conversion
		bool * recv_value[nP+1];
		bool * recv_mt_value[nP+1];
		block * recv_mac[nP+1];
		block * recv_mt_mac[nP+1];
		bool * send_value[nP+1];
		bool * send_mt_value[nP+1];
		block * send_mac[nP+1];
		block * send_mt_mac[nP+1];
		for(int i=1; i<=nP; ++i){
			recv_value[i] = new bool[num_in];
			recv_mt_value[i] = new bool[num_ands*3];
			recv_mac[i] = new block[num_in];
			recv_mt_mac[i] = new block[num_ands*3];
			send_value[i] = new bool[num_in];
			send_mt_value[i] = new bool[num_ands*3];
			send_mac[i] = new block[num_in];
			send_mt_mac[i] = new block[num_ands*3];
		}
		

		// verifiers send their shares to Dealer (party = 1)
		if (party != 1){
			io->send_data(1,value,num_in*sizeof(bool));
			io->send_data(1,mt_value, 3*num_ands*sizeof(bool));
			io->send_data(1,mac[1],num_in*sizeof(block));
			io->send_data(1,mt_mac[1],3*num_ands*sizeof(block));
			io->flush(1);


			io->recv_data(1,recv_value[1], num_in*sizeof(bool));
			io->recv_data(1,recv_mt_value[1],3*num_ands*sizeof(bool));
			io->recv_data(1,recv_mac[1],num_in*sizeof(block));
			io->recv_data(1,recv_mt_mac[1],3*num_ands*sizeof(block));

			// check the MAC tags
			block * tmp_in = new block[num_in];
			block * tmp_ands = new block[3*num_ands];
			memcpy(tmp_in, key[1], num_in*sizeof(block));
			memcpy(tmp_ands, mt_key[1], 3*num_ands*sizeof(block));
			for (int i = 0; i < num_in; ++i){
				if (recv_value[1][i])
					tmp_in[i] = tmp_in[i] ^ Delta;
			}
			if (!cmpBlock(tmp_in, recv_mac[1], num_in))
				cout << party << "\tMAC check fails for input wires" << endl;
			for (int i = 0; i < 3*num_ands; ++i){
				if (recv_mt_value[1][i])
					tmp_ands[i] = tmp_ands[i] ^ Delta;
			}
			if (!cmpBlock(tmp_ands, recv_mt_mac[1], num_ands*3))
				cout << party << "\tMAC check fails for AND gates" << endl;
			
			delete[] tmp_in;
			delete[] tmp_ands;

			// update the shares and local MAC keys
			update_add_cons(key, value, recv_value[1], Delta, num_in, party);
			update_add_cons(mt_key, mt_value, recv_mt_value[1], Delta, num_ands*3, party);

			//check_MAC_verifiers<nP>(io, mac, key, value, Delta, num_in, party);

		}
		else{
			// use multi-threads

			memcpy(send_value[1], value, num_in*sizeof(bool));
			memcpy(send_mt_value[1], mt_value, 3*num_ands*sizeof(bool));
			
			vector<future<void>>	 res;
			for(int i = 2; i <= nP; ++i) {
				int party2 = i;
				memcpy(send_mac[party2], mac[party2],num_in*sizeof(block));
				memcpy(send_mt_mac[party2], mt_mac[party2], 3*num_ands*sizeof(block));
				res.push_back(pool->enqueue([this, send_value, send_mt_value, send_mac, send_mt_mac, party2]() {
					io->send_data(party2, send_value[1], num_in*sizeof(bool));
					io->send_data(party2, send_mt_value[1], 3*num_ands*sizeof(bool));
					io->send_data(party2, send_mac[party2],num_in*sizeof(block));
					io->send_data(party2, send_mt_mac[party2],3*num_ands*sizeof(block));
					io->flush(party2);
				}));
				res.push_back(pool->enqueue([this, recv_value, recv_mt_value, recv_mac, recv_mt_mac, party2]() {
					io->recv_data(party2, recv_value[party2], num_in*sizeof(bool));
					io->recv_data(party2,recv_mt_value[party2], 3*num_ands*sizeof(bool));
					io->recv_data(party2,recv_mac[party2],num_in*sizeof(block));
					io->recv_data(party2,recv_mt_mac[party2],3*num_ands*sizeof(block));
				}));
			}
			joinNclean(res);

			// check the MAC tags
			block * tmp_in = new block[num_in];
			block * tmp_ands = new block[3*num_ands];
			for (int i=2; i<=nP;++i){
				int party2 = i;
				memcpy(tmp_in, key[party2], num_in*sizeof(block));
				memcpy(tmp_ands, mt_key[party2], 3*num_ands*sizeof(block));
				for (int j = 0; j < num_in; ++j){
					if (recv_value[party2][j])
						tmp_in[j] = tmp_in[j] ^ Delta;
				}
				if(!cmpBlock(tmp_in, recv_mac[party2], num_in))
					cout <<party << "\tMAC check fails for input wires!" << endl;
				for (int j = 0; j < 3*num_ands; ++j){
					if (recv_mt_value[party2][j])
						tmp_ands[j] = tmp_ands[j] ^ Delta;
				}
				if(!cmpBlock(tmp_ands, recv_mt_mac[party2], num_ands*3))
					cout <<party << "\tMAC check fails for AND gates!" << endl;
			}
			delete[] tmp_in;
			delete[] tmp_ands;

			// update its shares
			for (int i = 2; i <= nP; ++i){
				int party2 = i;
				for (int j = 0; j < num_in; ++j){
					value[j] = value[j] ^ recv_value[party2][j];
				}
				for (int j = 0; j < num_ands*3; ++j){
					mt_value[j] = mt_value[j] ^ recv_mt_value[party2][j];
				}
			}
			
		}

		for(int i=1; i<=nP; ++i){
			delete[] recv_value[i];
			delete[] recv_mt_value[i];
			delete[] recv_mac[i];
			delete[] recv_mt_mac[i];
			delete[] send_value[i];
			delete[] send_mt_value[i];
			delete[] send_mac[i];
			delete[] send_mt_mac[i];
		}
	}

    /* helper functions */
    void update_add_pos(block * MAC_1[nP+1], block * KEY_1[nP+1], bool * r_1, int pos_1, block * MAC_2[nP+1], block * KEY_2[nP+1], bool * r_2, int pos_2, int party) {
        if (party == 1)
            cout << "Not for dealer!" << endl;
        
        // add the share
        r_1[pos_1] = r_1[pos_1] ^ r_2[pos_2];
        
        // add the local MAC keys and MAC tags
        for (int i = 2; i <= nP; ++i){
            if (i == party)
                continue;
            MAC_1[i][pos_1] = MAC_1[i][pos_1] ^ MAC_2[i][pos_2];
            KEY_1[i][pos_1] = KEY_1[i][pos_1] ^ KEY_2[i][pos_2];
        }
        
    }

    void update_add_cons(block * KEY_1[nP+1], bool * r_1, bool * r_2, block Delta, int length, int party) {
        if (party == 1)
            cout << "Not for dealer!" << endl;
        
        if (party == 2){
            // add the share
            for (int i = 0; i< length; ++i){
                r_1[i] = r_1[i] ^ r_2[i];
            }
        }
        else{
            // update the local MAC key for party 2
            for (int i = 0; i< length; ++i){
                if (r_2[i]){
                    KEY_1[2][i] = KEY_1[2][i] ^ Delta;
                }
            }
        }
    }


    void update_add_cons_pos(block * KEY_1[nP+1], bool * r_1, bool r_2, block Delta, int pos, int party) {
        if (party == 1)
            cout << "Not for dealer!" << endl;
        
        if (party == 2){
            // add the share
            r_1[pos] = r_1[pos] ^ r_2;
        }
        else{
            // update the local MAC key for party 2
            if (r_2){
                KEY_1[2][pos] = KEY_1[2][pos] ^ Delta;
            }
        }
    }

	/* online protocol */
	void online(bool * input, bool * output){

		if (party == 1){
			// Dealer runs the round 1

			// for each input wire

			for(int i = 0; i < num_in; ++i){
				mask_input[i] = input[i] ^ value[i];
				wires[i] = input[i];
			}
			
			// evaluate the circuit
			int counter_and = 0;
			for(int i = 0; i < cf->num_gate; ++i){
				if (cf->gates[4*i+3] == AND_GATE){
					// compute the output of AND gate
					wires[cf->gates[4*i+2]] = wires[cf->gates[4*i]] & wires[cf->gates[4*i+1]]; 

					// asign the mask for AND gate
					mask_and[2*counter_and] = wires[cf->gates[4*i]] ^ mt_value[3*counter_and];
					mask_and[2*counter_and+1] = wires[cf->gates[4*i+1]] ^ mt_value[3*counter_and+1];
					counter_and++;
				}
				else if (cf->gates[4*i+3] == XOR_GATE){
					// compute the output of XOR gate
					wires[cf->gates[4*i+2]] = wires[cf->gates[4*i]] ^ wires[cf->gates[4*i+1]]; 
				}
				else{
					wires[cf->gates[4*i+2]] = wires[cf->gates[4*i]] ^ true;
				}
			}


            vector<future<void>> res;
			for(int i = 2; i <= nP; ++i) {
				int party2 = i;
				res.push_back(pool->enqueue([this, party2]() {
					io->send_data(party2, mask_input, num_in*sizeof(bool));
					io->send_data(party2, mask_and, 2*num_ands*sizeof(bool));
					io->flush(party2);
				}));
			}
			joinNclean(res);
			
			// set the output
			/*
			for(int i = 0; i < cf->num_output; ++i){
				output[i] = wires[cf->num_wire - cf->num_output + i];
			}*/

		}
		else{
			// verifiers run the round 2

			// get the proof (mask) from the dealer
			io->recv_data(1,mask_input,num_in);
			io->recv_data(1,mask_and,num_ands*2);

		
			// for input_wires
			for (int i = 2; i <= nP; ++i){
				if (i == party)
					continue;
				memcpy(wires_key[i], key[i], num_in*sizeof(block));
				memcpy(wires_mac[i], mac[i], num_in*sizeof(block));
			}
			memcpy(wires, value, num_in*sizeof(bool));
			// update the input wires with the mask_input
			update_add_cons(wires_key, wires, mask_input, Delta, num_in, party);



			// evaluate the circuit
			int counter_and = 0;
			for (int i=0; i < cf->num_gate; ++i){
				if (cf->gates[4*i+3] == AND_GATE){
					// set wires[cf->gates[4*i+2]] to be ci (i.e., mt_value[3*counter_and + 2])
					wires[cf->gates[4*i+2]] = mt_value[3*counter_and+2];
					for (int j = 2; j <= nP; j++){
						if (j == party)
							continue;
						wires_key[j][cf->gates[4*i+2]] = mt_key[j][3*counter_and+2];
						wires_mac[j][cf->gates[4*i+2]] = mt_mac[j][3*counter_and+2];
					}
					

					// update the wires, wires_mac, wires_key
					if (mask_and[2*counter_and]){
						int pos_1 = cf->gates[4*i+2];
						int pos_2 = 3*counter_and+1;
						update_add_pos(wires_mac, wires_key, wires, pos_1, mt_mac, mt_key, mt_value, pos_2,party);
					}
					if (mask_and[2*counter_and+1]){
						int pos_1 = cf->gates[4*i+2];
						int pos_2 = 3*counter_and;
						update_add_pos(wires_mac, wires_key, wires, pos_1, mt_mac, mt_key, mt_value, pos_2,party);
					}
					if ((mask_and[2*counter_and] and mask_and[2*counter_and+1])){
						int pos_1 = cf->gates[4*i+2];
						update_add_cons_pos(wires_key, wires, true, Delta, pos_1, party);
					}
					
					// set check_value
					check_value[2*counter_and] = wires[cf->gates[4*i]];
					check_value[2*counter_and+1] = wires[cf->gates[4*i+1]];
					for (int j = 2; j <= nP; j++){
						if (j == party)
							continue;
						check_key[j][2*counter_and] = wires_key[j][cf->gates[4*i]];
						check_mac[j][2*counter_and] = wires_mac[j][cf->gates[4*i]];
						check_key[j][2*counter_and+1] = wires_key[j][cf->gates[4*i+1]];
						check_mac[j][2*counter_and+1] = wires_mac[j][cf->gates[4*i+1]];
					}
					update_add_pos(check_mac, check_key, check_value, 2*counter_and, mt_mac, mt_key, mt_value, 3*counter_and, party);
					update_add_pos(check_mac, check_key, check_value, 2*counter_and+1, mt_mac, mt_key, mt_value, 3*counter_and+1, party);
					//make check_value the shares of 0
					if (mask_and[2*counter_and])
						update_add_cons_pos(check_key, check_value, true, Delta, 2*counter_and, party);
					if (mask_and[2*counter_and+1])
						update_add_cons_pos(check_key, check_value, true, Delta, 2*counter_and+1, party);
					counter_and++;

				}
				else if (cf->gates[4*i+3] == XOR_GATE){
					// compute the output of XOR gate
					wires[cf->gates[4*i+2]] = wires[cf->gates[4*i]] ^ wires[cf->gates[4*i+1]]; 
					// update the local MAC keys and MAC tags
					for (int j = 2; j <= nP; j++){
						if (party == j)
							continue;
						wires_key[j][cf->gates[4*i+2]] = wires_key[j][cf->gates[4*i]] ^ wires_key[j][cf->gates[4*i+1]]; 
						wires_mac[j][cf->gates[4*i+2]] = wires_mac[j][cf->gates[4*i]] ^ wires_mac[j][cf->gates[4*i+1]];
					}
					
				}
				else{
					// set the value
					wires[cf->gates[4*i+2]] = wires[cf->gates[4*i]]; 
					for (int j = 2; j <= nP; j++){
						if (party == j)
							continue;
						wires_key[j][cf->gates[4*i+2]] = wires_key[j][cf->gates[4*i]]; 
						wires_mac[j][cf->gates[4*i+2]] = wires_mac[j][cf->gates[4*i]];
					}

					// update
					update_add_cons_pos(wires_key, wires, true, Delta, cf->gates[4*i+2], party);

					
				}
			}

			bool * whole_value[nP+1];
			block * send_mac[nP+1];
			block * recv_mac[nP+1];
			for (int i = 2; i <= nP; ++i){
				whole_value[i] = new bool[num_ands*2+cf->num_output];
				send_mac[i] = new block[num_ands*2+cf->num_output];
				recv_mac[i] = new block[num_ands*2+cf->num_output];
			}

			// set the first 2*num_ands to be check_value
			memcpy(whole_value[party], check_value, num_ands*2*sizeof(bool));
			// set the last cf->n3 to be the value of output wires
			for (int i = num_ands*2; i < num_ands*2+cf->num_output; ++i){
				int j = i - num_ands*2;
				whole_value[party][i] = wires[cf->num_wire - cf->num_output + j];
			}
			// set the send_mac
			for (int i = 2; i <= nP; ++i){
				if (i == party)
					continue;
				for (int j = 0; j < num_ands*2; ++j){
					send_mac[i][j] = check_mac[i][j];
				}
				for (int j = num_ands*2; j < num_ands*2+cf->num_output; ++j){
					int k =  j - num_ands*2;
					send_mac[i][j] = wires_mac[i][cf->num_wire - cf->num_output + k];
				}
			}

			// communication between the verifiers
			
			vector<future<void>>	res;
			for(int i = 2; i <= nP; ++i) for(int j = 2; j <= nP; ++j) if( (i < j) and (i == party or j == party) ) {
				int party2 = i + j - party;
				res.push_back(pool->enqueue([this, whole_value, send_mac, party2]() {
					io->send_data(party2, whole_value[party], (num_ands*2+cf->num_output)*sizeof(bool));
					io->send_data(party2, send_mac[party2], (num_ands*2+cf->num_output)*sizeof(block));
					io->flush(party2);
				}));
				res.push_back(pool->enqueue([this, whole_value, recv_mac, party2]() {
					io->recv_data(party2, whole_value[party2], (num_ands*2+cf->num_output)*sizeof(bool));
					io->recv_data(party2, recv_mac[party2], (num_ands*2+cf->num_output)*sizeof(block));
				}));
			}	
			joinNclean(res);
			
			// check the consistency between the received value and MACs
			for (int i = 2; i <= nP; ++i){
				if (i == party)
					continue;
				for (int j = 0; j < num_ands*2; ++j){
					block tmp = check_key[i][j];
					if (whole_value[i][j])
						tmp = tmp ^ Delta;
					if(!cmpBlock( &tmp, &recv_mac[i][j],1))
						cout << "MAC check fails" << endl;
				}
				
				for (int j = num_ands*2; j < num_ands*2+cf->num_output; ++j){
					int k = j - num_ands*2;
					block tmp = wires_key[i][cf->num_wire - cf->num_output + k];
					if (whole_value[i][j]){
						tmp = tmp ^ Delta;
					}
					if(!cmpBlock( &tmp, &recv_mac[i][j],1))
						cout << "MAC check fails" << endl;
				}
			}

			// reconstruct the share
			for(int i = 2; i <= nP; ++i){
				if (i ==  party)
					continue;
				for (int j = 0; j < num_ands*2 + cf->num_output ; ++j){
					whole_value[party][j] =  whole_value[party][j] ^ whole_value[i][j];
				}
			}

			bool flag = true;
			for (int i = 0; i < num_ands*2; ++i){
				if(whole_value[party][i]){
					flag = false;
				}
			}
			
			if (!flag)
				cout << "Party " << party << "fails\n";

			
			// set the output
			/*
			for(int i = 0; i < cf->num_output; ++i){
				output[i] = whole_value[party][num_ands*2 + i];
			}*/
			


			for (int i=2; i<=nP;++i){
				delete[] whole_value[i];
				delete[] send_mac[i];
				delete[] recv_mac[i];
			}
		}
	}

};