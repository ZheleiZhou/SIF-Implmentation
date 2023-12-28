#pragma once

#include "emp-agmpc/emp-agmpc.h"
#include "emp-tool/emp-tool.h"
#include "emp-tool/utils/f2k.h"
#include "mv-svole/mvsvole_bool_pcg.h"
#include <cstdint>
#include <cstring>

using namespace std;
using namespace emp;

//1-round SIF over binary field
template<int nP>
class OneRound_SIF_Bool_PCG{ public:
	
	MVSVOLEBOOL_PCG<nP>* fvolemp = nullptr;

	// single values
	block* mac[nP+1];
	block* key[nP+1];
	bool* value;

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
	
	// for multiplication checks
	block * mt_A[nP+1];
	block * mt_B;

	block Delta;
	PRP prp;
	PRG prg;

	// vole over extension fields (pair-wise)
	GaloisFieldPacking pack;
	block * value_ext[nP+1];
	block * mac_ext[nP+1];
	block * key_ext[nP+1];

	OneRound_SIF_Bool_PCG(NetIOMP<nP> * io[2], ThreadPool * pool, int party, BristolFashion * cf, bool * _delta = nullptr, int ssp=40) {
		this->party = party;
		this->io = io[0];
		// cf is the circuit
		this->cf = cf;
		this->pool = pool;
		
		for(int i = 0; i < cf->num_gate; ++i) {
			if (cf->gates[4*i+3] == AND_GATE)
				++num_ands;
		}
		num_in = cf->num_input;

		
		
		// 2*ssp for consistency check,  128*(np-1) for conversion
		total_pre = num_in + num_ands + 2*ssp + (nP-1)*128+101;
		
		fvolemp = new MVSVOLEBOOL_PCG<nP>(io[1], pool, party);

		if (party != 1)
			Delta = fvolemp->Delta;

		for(int i  = 1; i <= nP; ++i) {
			key[i] = new block[total_pre];
			mac[i] = new block[total_pre];
			wires_key[i] = new block[cf->num_wire];
			wires_mac[i] = new block[cf->num_wire];
			mt_A[i] = new block[num_ands*2];
			mac_ext[i] = new block[1];
			key_ext[i] = new block[1];
			value_ext[i] = new block[1];
		}
		mt_B = new block[num_ands];
		value = new bool[total_pre];
		wires = new bool[cf->num_wire];

		mask_input = new bool[num_in];
		mask_and = new bool[num_ands];

	}
	~OneRound_SIF_Bool_PCG() {
		for(int i = 1; i <= nP; ++i) {
			delete[] key[i];
			delete[] mac[i];
			delete[] wires_key[i];
			delete[] wires_mac[i];
			delete[] mac_ext[i];
			delete[] key_ext[i];
			delete[] value_ext[i];
		}
		delete[] value;
		delete[] wires;
		delete[] mask_input;
		delete[] mask_and;
	}
    
	void Preprocess(){
		if (party == 1){
			fvolemp->computeVOLE_mal(mac, value, total_pre);
		}	
		else{
			fvolemp->computeVOLE_mal(key, value, total_pre);
		}

		if(party == 1){
			block * ope_data = new block[128];
			bool * ope_bool = new bool[128];
			
			for (int i = 2; i <= nP; ++i){
				int off_index = num_in + num_ands + (i-2)*128;
				memcpy(ope_data, mac[i]+off_index, 128*sizeof(block));
				memcpy(ope_bool, value+off_index, 128*sizeof(bool));

				uint64_t ch[2];
				for(int j = 0; j < 2; ++j){
					if (ope_bool[64*j+63])
						ch[j] = 1;
					else
					 	ch[j] = 0;

					for(int k = 62; k >= 0; k--){
						ch[j] <<= 1;
						if(ope_bool[64*j + k])
							ch[j]++;
					}
				}
				value_ext[i][0] = makeBlock(ch[1], ch[0]);
				pack.packing(mac_ext[i], ope_data);
				
			}
			delete[] ope_data;
			delete[] ope_bool;
		}
		else{
			block * ope_data = new block[128];
			int off_index = num_in + num_ands + (party-2)*128;
			memcpy(ope_data, key[1]+off_index, 128*sizeof(block));
	
			pack.packing(key_ext[1], ope_data);
			delete[] ope_data;
		}

	}

	void online(bool * input, bool * output){
		block seed;

		if (party == 1){
			// Dealer generates the ''proof''

			// for each input wire
			
			for(int i = 0; i < num_in; ++i){
				mask_input[i] = input[i] ^ value[i];
				wires[i] = input[i];
				for (int j=2; j<=nP; ++j){
					wires_mac[j][i] = mac[j][i];
				}
			}
			
			// evaluate the circuit
			int counter_and = 0;
			for(int i = 0; i < cf->num_gate; ++i){
				if (cf->gates[4*i+3] == AND_GATE){
					// compute the output of AND gate
					wires[cf->gates[4*i+2]] = wires[cf->gates[4*i]] & wires[cf->gates[4*i+1]]; 

					// asign the mask for AND gate
					mask_and[counter_and] = wires[cf->gates[4*i+2]] ^ value[num_in+counter_and];

					// compute the values for multiplication checks
					for (int j = 2; j <= nP; j++){
						wires_mac[j][cf->gates[4*i+2]] = mac[j][num_in+counter_and];

						gfmul(wires_mac[j][cf->gates[4*i]], wires_mac[j][cf->gates[4*i+1]], &mt_A[j][2*counter_and]);

						mt_A[j][2*counter_and+1] = zero_block;
						if (wires[cf->gates[4*i]] == true)
							mt_A[j][2*counter_and+1] = mt_A[j][2*counter_and+1] ^ wires_mac[j][cf->gates[4*i+1]];
						if (wires[cf->gates[4*i+1]] == true)
							mt_A[j][2*counter_and+1] =  mt_A[j][2*counter_and+1] ^ wires_mac[j][cf->gates[4*i]];
						mt_A[j][2*counter_and+1] =  mt_A[j][2*counter_and+1] ^ wires_mac[j][cf->gates[4*i+2]];
					}

					counter_and++;
				}
				else if (cf->gates[4*i+3] == XOR_GATE){
					// compute the output of XOR gate
					wires[cf->gates[4*i+2]] = wires[cf->gates[4*i]] ^ wires[cf->gates[4*i+1]]; 

					// compute the mac
					for (int j = 2; j <= nP; j++){
						wires_mac[j][cf->gates[4*i+2]] = wires_mac[j][cf->gates[4*i]] ^ wires_mac[j][cf->gates[4*i+1]]; 
					}
				}
				else{
					wires[cf->gates[4*i+2]] = wires[cf->gates[4*i]] ^ true;

					// compute the mac
					for (int j = 2; j <= nP; j++){
						wires_mac[j][cf->gates[4*i+2]] = wires_mac[j][cf->gates[4*i]]; 
					}
				}
			}

			// using seed to generate different chi's
			seed = Hash::hash_for_block(mask_and, num_ands);
			block * chi = new block[num_ands];
			PRG prg_chi = new PRG;
			prg_chi.reseed(&seed);
			prg_chi.random_block(chi, num_ands);

			// compute the multiplication checks
			for(int i = 2; i <= nP; ++i){
				block tmp_check;
				block com_mt_A[2];
				com_mt_A[0] = zero_block;
				com_mt_A[1] = zero_block;
				for(int j = 0; j < num_ands; ++j){
					gfmul(chi[j], mt_A[i][2*j+1], &tmp_check);
					com_mt_A[1] = com_mt_A[1] ^ tmp_check;

					gfmul(chi[j], mt_A[i][2*j], &tmp_check);
					com_mt_A[0] = com_mt_A[0] ^ tmp_check;
				}
				value_ext[i][0] = value_ext[i][0] ^ com_mt_A[1];
				mac_ext[i][0] = mac_ext[i][0] ^ com_mt_A[0];
			}

			// send the proof (mask) to the verifiers
			vector<future<void>> res;
			for(int i = 2; i <= nP; ++i) {
				int party2 = i;
				res.push_back(pool->enqueue([this, party2]() {
					io->send_data(party2, mask_input, num_in*sizeof(bool));
					io->send_data(party2, mask_and, num_ands*sizeof(bool));
					io->send_data(party2, value_ext[party2], sizeof(block));
					io->send_data(party2, mac_ext[party2], sizeof(block));
					io->flush(party2);
				}));
			}
			joinNclean(res);

			delete[] chi;

		}
		else{
			// verifiers' protocol 

			// get the proof (mask) from the dealer
			block recv_value_ext, recv_mac_ext;
			
			io->recv_data(1,mask_input,num_in*sizeof(bool));
			io->recv_data(1,mask_and,num_ands*sizeof(bool));
			io->recv_data(1, &recv_value_ext, sizeof(block));
			io->recv_data(1, &recv_mac_ext, sizeof(block));
			

			// evaluate the circuit
			for (int i = 0; i < num_in; i++){
				wires_key[1][i] = key[1][i];
				if(mask_input[i])
					wires_key[1][i] = wires_key[1][i] ^ Delta;
			}

			int counter_and = 0;
			for(int i = 0; i < cf->num_gate; ++i){
				if (cf->gates[4*i+3] == AND_GATE){
					// compute the local mac key of the output of AND gate
					wires_key[1][cf->gates[4*i+2]] = key[1][num_in+counter_and];
					if(mask_and[counter_and])
						wires_key[1][cf->gates[4*i+2]] = wires_key[1][cf->gates[4*i+2]] ^ Delta;

					// compute the values for multiplication checks
					block tmp_check;
					gfmul(wires_key[1][cf->gates[4*i]], wires_key[1][cf->gates[4*i+1]], &tmp_check);
					mt_B[counter_and] = tmp_check;
					gfmul(wires_key[1][cf->gates[4*i+2]], Delta, &tmp_check);
					mt_B[counter_and] = mt_B[counter_and] ^ tmp_check;

					counter_and++;
				}
				else if (cf->gates[4*i+3] == XOR_GATE){

					// compute the local mac key
					wires_key[1][cf->gates[4*i+2]] = wires_key[1][cf->gates[4*i]] ^ wires_key[1][cf->gates[4*i+1]]; 
				}
				else{
					// compute the local mac key
					wires_key[1][cf->gates[4*i+2]] = wires_key[1][cf->gates[4*i]] ^ Delta; 
				}
			}

			// using seed to generate different chi's
			seed = Hash::hash_for_block(mask_and, num_ands);
			block * chi = new block[num_ands];
			PRG prg_chi = new PRG;
			prg_chi.reseed(&seed);
			prg_chi.random_block(chi, num_ands);

			// compute the multiplication checks
			block tmp_check;
			block com_mt_B = zero_block;
			for (int i = 0; i < num_ands; ++i){
				gfmul(mt_B[i], chi[i], &tmp_check);
				com_mt_B = com_mt_B ^ tmp_check;
			}
			key_ext[1][0] = key_ext[1][0] ^ com_mt_B;

			// check the multiplication gates
			gfmul(recv_value_ext, Delta, &tmp_check);
			tmp_check = tmp_check ^ recv_mac_ext;
			if(!cmpBlock(&tmp_check, &key_ext[1][0], 1))
				cout << "Multiplication checks fail!" << endl;

		}

		
	}


	/*  debug function */
	void debug_conversion(){
		if (party == 1){
			for(int i = 2; i <= nP; ++i){
				io->send_data(i, value_ext[i], sizeof(block));
				io->send_data(i, mac_ext[i], sizeof(block));
			}
		}
		else{
			io->recv_data(1, value_ext[1], sizeof(block));
			io->recv_data(1, mac_ext[1], sizeof(block));


			block tmp;
			// tmp = value_ext[1][0] * Delta
			gfmul(value_ext[1][0], Delta, &tmp);
			tmp = tmp ^ mac_ext[1][0];
			if(!cmpBlock(&tmp, &key_ext[1][0], 1))
				cout << "conversion failed!" << endl;
		}
	}

	void debug_circuits_evaluation(){
		if(party == 1){
			for(int i = 2; i <= nP; ++i){
				io->send_data(i, wires, cf->num_wire*sizeof(bool));
				io->send_data(i, wires_mac[i], cf->num_wire*sizeof(block));
			}
		}
		else{
			bool * recv_wires = new bool[cf->num_wire];
			block * recv_wire_mac = new block[cf->num_wire];
			io->recv_data(1, recv_wires, cf->num_wire*sizeof(bool));
			io->recv_data(1, recv_wire_mac, cf->num_wire*sizeof(block));

			for(int i = 0; i < num_in; ++i){
				block tmp_check = recv_wire_mac[i];
				if(recv_wires[i])
					tmp_check = tmp_check ^ Delta;

				if(!cmpBlock(&tmp_check, &wires_key[1][i], 1))
					cout << "Input phase is wrong!" << endl;
			}

			for(int i = 0; i < cf->num_gate; ++i){
				if (cf->gates[4*i+3] == AND_GATE){
					block tmp_check = recv_wire_mac[cf->gates[4*i+2]];
					if(recv_wires[cf->gates[4*i+2]])
						tmp_check = tmp_check ^ Delta;
					
					if(!cmpBlock(&tmp_check, &wires_key[1][cf->gates[4*i+2]], 1)){
						cout << "AND gates are wrong!" << endl;
						// flag[0] = true;
						// first_wrong[0] = i;
					}
				}
				else if (cf->gates[4*i+3] == XOR_GATE){
					block tmp_check = recv_wire_mac[cf->gates[4*i+2]];
					if(recv_wires[cf->gates[4*i+2]])
						tmp_check = tmp_check ^ Delta;

					if(!cmpBlock(&tmp_check, &wires_key[1][cf->gates[4*i+2]], 1)){
						cout << "XOR gates are wrong!" << endl;
					}
						
				}
				else{
					block tmp_check = recv_wire_mac[cf->gates[4*i+2]];
					if(recv_wires[cf->gates[4*i+2]])
						tmp_check = tmp_check ^ Delta;

					if(!cmpBlock(&tmp_check, &wires_key[1][cf->gates[4*i+2]], 1)){
						cout << "NOT gates are wrong!" << endl;
					}
						
				}
			}
		}
	}

	void debug_multiplication_check(){
		if(party == 1){
			for(int i = 2; i <= nP; ++i){
				io->send_data(i, mt_A[i], 2*num_ands*sizeof(block));
				io->send_data(i, value_ext[i], sizeof(block));
				io->send_data(i, mac_ext[i], sizeof(block));
			}
		}
		else{
			block * recv_mt_A = new block[2*num_ands];
			block recv_value_ext, recv_mac_ext;
			io->recv_data(1, recv_mt_A, 2*num_ands*sizeof(block));
			io->recv_data(1, &recv_value_ext, sizeof(block));
			io->recv_data(1, &recv_mac_ext, sizeof(block));

			// check extension VOLE
			block tmp_ext;
			gfmul(recv_value_ext, Delta, &tmp_ext);
			tmp_ext = tmp_ext ^ recv_mac_ext;
			if(!cmpBlock(&tmp_ext, &key_ext[1][0], 1))
				cout << "VOLE over extension field fails" << endl;

			for(int i = 0; i <num_ands; ++i){
				block tmp;
				gfmul(recv_mt_A[2*i+1], Delta, &tmp);
				tmp = tmp ^ recv_mt_A[2*i];
				if(!cmpBlock(&tmp, &mt_B[i], 1)){
					cout << "Each MT fails" << endl;
				}
			}

			block * chi = new block[num_ands];
			PRG prg_chi;
			prg_chi.random_block(chi, num_ands);
			block com_mt_A[2];
			com_mt_A[0] = zero_block;
			com_mt_A[1] = zero_block;
			block com_mt_b = zero_block;

			for(int i = 0; i < num_ands; ++i){
				block tmp;
				gfmul(recv_mt_A[2*i], chi[i], &tmp);
				com_mt_A[0] = com_mt_A[0] ^ tmp;
				gfmul(recv_mt_A[2*i+1], chi[i], &tmp);
				com_mt_A[1] = com_mt_A[1] ^ tmp;
				gfmul(mt_B[i], chi[i], &tmp);
				com_mt_b = com_mt_b ^ tmp;
			}

			block tmp_check;
			gfmul(com_mt_A[1], Delta, &tmp_check);
			tmp_check = tmp_check ^ com_mt_A[0];
			if(!cmpBlock(&tmp_check, &com_mt_b, 1))
				cout << "Random combination of MT fails" << endl;
			
		}
	}
};