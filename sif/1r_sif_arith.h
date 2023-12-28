#pragma once

#include "emp-agmpc/emp-agmpc.h"
#include "emp-tool/emp-tool.h"
#include "emp-zk/emp-vole/utility.h"
#include "emp-zk/emp-zk-arith/ostriple.h"
#include "mv-svole/mvsvole_arith.h"
#include <cstdint>
#include <cstring>

using namespace std;
using namespace emp;

//1-round SIF over large prime field
template<int nP>
class OneRound_SIF_Arith{ public:
	
	MVVOLEarith<nP>* fvolemp = nullptr;
	
	
	// single values
	__uint128_t* mac[nP+1];
	__uint128_t* key[nP+1];


	// wires values
	uint64_t * wires;
	uint64_t * wires_mac[nP+1];
	uint64_t * wires_key;

	BristolFashion * cf;
	NetIOMP<nP> * io;
	int num_ands = 0, num_in;
	int party, total_pre, ssp;
	ThreadPool * pool;

	uint64_t * mask_input;
	uint64_t * mask_and;
	
	// for multiplication checks
	uint64_t * mt_A[nP+1];
	uint64_t * mt_B;

	__uint128_t Delta;
	PRP prp;
	PRG prg;

	// vole over extension fields (pair-wise)
	uint64_t value_ext[nP+1];
	uint64_t mac_ext[nP+1];
	uint64_t key_ext;

    OneRound_SIF_Arith(NetIOMP<nP> * io[2], ThreadPool * pool, int party, BristolFashion * cf, bool * _delta = nullptr) {
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

		// 1 for consistency check, 1*nP for conversion
		total_pre = num_in + num_ands + 1 + (nP-1);
		
		fvolemp = new MVVOLEarith<nP>(io[1], pool, party);


		Delta = fvolemp->Delta;


		for(int i  = 1; i <= nP; ++i) {
			key[i] = new __uint128_t[total_pre];
			mac[i] = new __uint128_t[total_pre];
			wires_mac[i] = new uint64_t[cf->num_wire];
			mt_A[i] = new uint64_t[num_ands*2];
		}
		mt_B = new uint64_t[num_ands];
		wires = new uint64_t[cf->num_wire];
		wires_key = new uint64_t[cf->num_wire];
		mask_input = new uint64_t[num_in];
		mask_and = new uint64_t[num_ands];

	}
	~OneRound_SIF_Arith() {
		for(int i = 1; i <= nP; ++i) {
			delete[] key[i];
			delete[] mac[i];	
			delete[] wires_mac[i];
		}
		delete[] mask_input;
		delete[] mask_and;
		delete[] wires;
		delete[] wires_key;
	}
	


	void Preprocess(){
		if (party == 1){
			fvolemp->computeVOLE_mal(mac, total_pre);

			for(int i = 2; i <= nP; ++i){
				int off_index = num_in + num_ands + i-2;
				value_ext[i] = HIGH64(mac[i][off_index]);
				mac_ext[i] = LOW64(mac[i][off_index]);
			}
		}
		else{
			fvolemp->computeVOLE_mal(key, total_pre);
			int off_index = num_in + num_ands + party-2;
			key_ext = LOW64(key[1][off_index]);
		}

	}

	
	void online(uint64_t * input, uint64_t * output){
		
		block seed; 

		if (party == 1){
			// Dealer generates the ''proof''

			// for each input wire
			for(int i = 0; i < num_in; ++i){
				wires[i] = input[i];
				uint64_t minus_input = PR - input[i];
				mask_input[i] = add_mod(HIGH64(mac[2][i]), minus_input);
				for (int j=2; j<=nP; ++j){
					wires_mac[j][i] = LOW64(mac[j][i]);
				}
			}
			
			
			
			// evaluate the circuit
			int counter_and = 0;
			for(int i = 0; i < cf->num_gate; ++i){
				if (cf->gates[4*i+3] == AND_GATE){
					// compute the output of AND gate
					uint64_t w_alpha = wires[cf->gates[4*i]];
					uint64_t w_beta = wires[cf->gates[4*i+1]];
					uint64_t w_gamma = mult_mod(w_alpha, w_beta); 
					wires[cf->gates[4*i+2]] = w_gamma;
					
					// asign the mask for AND gate
					mask_and[counter_and] = PR - w_gamma;
					mask_and[counter_and] = add_mod(mask_and[counter_and], HIGH64(mac[2][num_in+counter_and]));

					// compute the values for multiplication checks
					for (int j = 2; j <= nP; j++){
						uint64_t m_alpha = wires_mac[j][cf->gates[4*i]];
						uint64_t m_beta = wires_mac[j][cf->gates[4*i+1]];
						uint64_t m_gamma = LOW64(mac[j][num_in+counter_and]);

						wires_mac[j][cf->gates[4*i+2]] = m_gamma; 

						mt_A[j][2*counter_and] = mult_mod(m_alpha, m_beta);
						mt_A[j][2*counter_and+1] = PR - m_gamma;
						mt_A[j][2*counter_and+1] = add_mod( mult_mod(w_alpha, m_beta) , mt_A[j][2*counter_and+1]);
						mt_A[j][2*counter_and+1] = add_mod( mult_mod(w_beta, m_alpha) , mt_A[j][2*counter_and+1]);
					}

					counter_and++;
				}
				else if (cf->gates[4*i+3] == XOR_GATE){
					// compute the output of XOR gate
					wires[cf->gates[4*i+2]] = add_mod(wires[cf->gates[4*i]], wires[cf->gates[4*i+1]]);
					
					for (int j = 2; j <= nP; ++j){
						wires_mac[j][cf->gates[4*i+2]] = add_mod(wires_mac[j][cf->gates[4*i]], wires_mac[j][cf->gates[4*i+1]]);
						  
					}
				}
			}
			
			// using seed to generate different chi's
			seed = Hash::hash_for_block(mask_and, num_ands*sizeof(uint64_t));
			uint64_t * chi = new uint64_t[num_ands];
			PRG prg_chi = new PRG;
			prg_chi.reseed(&seed);
			prg_chi.random_data(chi, num_ands*sizeof(uint64_t));
			for(int i = 0; i < num_ands; ++i){
				chi[i] = mod(chi[i]);
			}

			// compute the multiplication checks
			for (int i = 2; i <= nP; ++i){
				for (int j = 0; j < num_ands; ++j){
					uint64_t tmp = mult_mod(mt_A[i][2*j], chi[j]);
					mac_ext[i] = add_mod(mac_ext[i], tmp);
					tmp = mult_mod(mt_A[i][2*j+1], chi[j]);
					value_ext[i] = add_mod(value_ext[i], tmp);
				}
			}

			
			// send the proof (mask) to the verifiers
			vector<future<void>> res;
			for(int i = 2; i <= nP; ++i) {
				int party2 = i;
				res.push_back(pool->enqueue([this, party2]() {
					io->send_data(party2, mask_input, num_in*sizeof(uint64_t));
					io->send_data(party2, mask_and, num_ands*sizeof(uint64_t));
					io->send_data(party2, &value_ext[party2], sizeof(uint64_t));
					io->send_data(party2, &mac_ext[party2], sizeof(uint64_t));
					io->flush(party2);
				}));
			}
			joinNclean(res);
			

			

		}
		else{
			
			// verifiers' protocol 

			// get the proof (mask) from the dealer
			uint64_t recv_value_ext, recv_mac_ext;

			io->recv_data(1,mask_input,num_in*sizeof(uint64_t));
			io->recv_data(1,mask_and,num_ands*sizeof(uint64_t));
			io->recv_data(1, &recv_value_ext, sizeof(uint64_t));
			io->recv_data(1, &recv_mac_ext, sizeof(uint64_t));

			
			// evaluate the circuit
			for (int i = 0; i < num_in; i++){
				uint64_t tmp = mult_mod(mask_input[i], Delta);
				wires_key[i] =  add_mod(key[1][i], tmp);
			}

			int counter_and = 0;
			for(int i = 0; i < cf->num_gate; ++i){
				if (cf->gates[4*i+3] == AND_GATE){
					// compute the multiplicaiton gate

					uint64_t tmp = mult_mod(mask_and[counter_and], Delta);
					wires_key[cf->gates[4*i+2]] = add_mod(key[1][num_in+counter_and], tmp);
					

					// compute the values for multiplication checks
					uint64_t tmp_key = mult_mod(wires_key[cf->gates[4*i]], wires_key[cf->gates[4*i+1]]);
					uint64_t tmp_key2 = mult_mod(wires_key[cf->gates[4*i+2]], Delta);
					mt_B[counter_and] = add_mod(tmp_key, tmp_key2);

					counter_and++;
				}
				else if (cf->gates[4*i+3] == XOR_GATE){

					// compute the add gate
					wires_key[cf->gates[4*i+2]] = add_mod(wires_key[cf->gates[4*i]], wires_key[cf->gates[4*i+1]]);
				}
			}
			
			// using seed to generate different chi's
			seed = Hash::hash_for_block(mask_and, num_ands*sizeof(uint64_t));
			uint64_t * chi = new uint64_t[num_ands];
			PRG prg_chi = new PRG;
			prg_chi.reseed(&seed);
			prg_chi.random_data(chi, num_ands*sizeof(uint64_t));
			for(int i = 0; i < num_ands; ++i){
				chi[i] = mod(chi[i]);
			}

			// compute the multiplication checks
			for (int i = 0; i < num_ands; ++i){
				uint64_t tmp = mult_mod(mt_B[i], chi[i]);
				key_ext = add_mod(tmp, key_ext);  
			}

			// check the multiplication gates
			uint64_t tmp_check = mult_mod(recv_value_ext, Delta);
			tmp_check = add_mod(key_ext, tmp_check);
			if(tmp_check != recv_mac_ext)
				cout << "party " << party << " : multiplication checks fail" << endl;

			
		}
	}
	
	/* debug functions */
	void debug_extension_vole(){
		if(party == 1){
			for(int i = 2; i <= nP; ++i){
				io->send_data(i, &value_ext[i], sizeof(uint64_t));
				io->send_data(i, &mac_ext[i], sizeof(uint64_t));
			}
		}
		else{
			uint64_t recv_value, recv_mac;
			io->recv_data(1, &recv_value, sizeof(uint64_t));
			io->recv_data(1, &recv_mac, sizeof(uint64_t));

			uint64_t tmp = mult_mod(recv_value, Delta);
			tmp = add_mod(tmp, key_ext);
			if(tmp != recv_mac)
				cout << "Extension VOLE fails" << endl;
		}
	}


	void debug_circuit_evaluation(){
		if(party == 1){
			for(int i = 2; i <= nP; ++i){
				io->send_data(i, wires, cf->num_wire*sizeof(uint64_t));
				io->send_data(i, wires_mac[i], cf->num_wire*sizeof(uint64_t));
			}
		}
		else{
			uint64_t * recv_wire = new uint64_t[cf->num_wire];
			uint64_t * recv_wire_mac = new uint64_t[cf->num_wire];
			io->recv_data(1, recv_wire, cf->num_wire*sizeof(uint64_t));
			io->recv_data(1, recv_wire_mac, cf->num_wire*sizeof(uint64_t));

			// check the input phase
			for(int i = 0; i < num_in; ++i){
				uint64_t tmp = mult_mod(recv_wire[i], Delta);
				tmp = add_mod(tmp, wires_key[i]);
				if(tmp != recv_wire_mac[i])
					cout << "Party " << party << ": "<< i <<"-th input wire fails" << endl;
			}

			for(int i = 0; i < cf->num_gate; ++i){
				if (cf->gates[4*i+3] == AND_GATE){
					uint64_t tmp = mult_mod(recv_wire[cf->gates[4*i+2]], Delta);
					tmp = add_mod(tmp, wires_key[cf->gates[4*i+2]]);
					if(tmp != recv_wire_mac[cf->gates[4*i+2]])
						cout << "Party " << party << ": "<< i <<"-th multiplication gate's output wire fails" << endl;
				}
				else if (cf->gates[4*i+3] == XOR_GATE){
					uint64_t tmp = mult_mod(recv_wire[cf->gates[4*i+2]], Delta);
					tmp = add_mod(tmp, wires_key[cf->gates[4*i+2]]);
					if(tmp != recv_wire_mac[cf->gates[4*i+2]])
						cout << "Party " << party << ": "<< i <<"-th addition gate's output wire fails" << endl;
				}
			}


		}
	}

	void debug_multiplication_check(){
		if (party == 1){
			for(int i = 2; i <= nP; ++i){
				io->send_data(i, mt_A[i], num_ands*2*sizeof(uint64_t));
			}
		}
		else{
			uint64_t * recv_mt_A = new uint64_t[2*num_ands*sizeof(uint64_t)];
			io->recv_data(1, recv_mt_A, 2*num_ands*sizeof(uint64_t));

			for(int i = 0; i < num_ands; ++i){
				uint64_t tmp = mult_mod(recv_mt_A[2*i+1], Delta);
				tmp = add_mod(tmp, mt_B[i]);
				if(tmp != recv_mt_A[2*i])
					cout << "Party " << party << " : "<< i << "-th MT fails" << endl;
			}

			PRG prg_chi;
			uint64_t * chi = new uint64_t[num_ands];
			prg_chi.random_data(chi, num_ands*sizeof(uint64_t));
			for(int i = 0; i < num_ands; ++i){
				chi[i] = mod(chi[i]);
			}
			uint64_t com_mt_A0 = 0;
			uint64_t com_mt_A1 = 0;
			uint64_t com_mt_B = 0;

			uint64_t tmp;
			for(int i = 0; i < num_ands; ++i){
				tmp = mult_mod(recv_mt_A[2*i], chi[i]);
				com_mt_A0 = add_mod(com_mt_A0, tmp);
				tmp = mult_mod(recv_mt_A[2*i+1], chi[i]);
				com_mt_A1 = add_mod(com_mt_A1, tmp);
				tmp = mult_mod(mt_B[i], chi[i]);
				com_mt_B = add_mod(com_mt_B, tmp);
			}
			tmp = mult_mod(com_mt_A1, Delta);
			tmp = add_mod(tmp, com_mt_B);
			if(tmp != com_mt_A0)
				cout << party << " : random combination of MT fails" << endl;
		}
	}

};