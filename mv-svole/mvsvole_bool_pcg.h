#pragma once

#include "emp-tool/emp-tool.h"
#include "emp-ot/emp-ot.h"
#include "emp-agmpc/netmp.h"
#include "emp-agmpc/helper.h"
#include "emp-tool/utils/block.h"


using namespace std;

// mv-svole over binary field via ProgFerretCOT (PCG-style)
template<int nP>
class MVSVOLEBOOL_PCG { public:
	FerretCOT<NetIO> * ferret[nP+1];
	NetIOMP<nP> *io;
	ThreadPool * pool;
	int party;
	PRG prg;
	block Delta;
	Hash hash;
	int ssp;
    block fixed_seed;

	MVSVOLEBOOL_PCG(NetIOMP<nP>* io, ThreadPool * pool, int party, int ssp = 40) {
		this->io = io;
		this->pool = pool;
		this->party = party;
    	this->ssp = ssp;

       

		if (party == 1){
			for(int i = 2; i <= nP; ++i){
				// acts as COT receiver
				ferret[i] = new FerretCOT<NetIO>(2,1,&io->get(i, false), true);
			}
		}
		else{
			// act as COT sender
			ferret[1] = new FerretCOT<NetIO>(1,1, &io->get(1, false), true);
            Delta = ferret[1]->Delta;
		}


	}
	~MVSVOLEBOOL_PCG() {
		if (party == 1){
			for (int i=2; i <= nP; ++i){
				delete ferret[i];
			}
		}
		else{
			delete ferret[1];
		}
	}
	
	void computeVOLE_semi(block * output[nP+1], bool* data, int length) {

		if (party == 1){
            // acts as COT receiver

            prg.random_bool(data, length);

			vector<future<void>> res;
			for (int i = 2; i<=nP; ++i){
				int party2 = i;
				res.push_back(pool->enqueue([this, output, data, length, party2]() {
					ferret[party2]->recv_cot(output[party2], data, length);
					io->flush(party2);
				}));
			}
			joinNclean(res);

            // vector<future<void>> res;
			// for (int i = 2; i<=nP; ++i){
			// 	int party2 = i;
			// 	res.push_back(pool->enqueue([this, output, length, party2]() {
			// 		ferret[party2]->rcot(output[party2], length);
			// 		io->flush(party2);
			// 	}));
			// }
			// joinNclean(res);

            // for(int i = 0; i < length; ++i){
            //     data[i] = getLSB(output[2][i]);
            // }

        
		}
		else{
            // acts as COT sender
			ferret[1]->send_cot(output[1], length);

            // ferret[1]->rcot(output[1], length);
			io->flush(1);

		}
		
	}

    void computeVOLE_mal(block * output[nP+1], bool* data, int length){

        computeVOLE_semi(output, data, length);

        block coin = sampleRandom(io, &prg, pool, party); 
		bool * s = new bool[length*ssp];
		PRG prg2(&coin);
		prg2.random_bool(s, length*ssp);

        bool * u = new bool[ssp];
        
        if (party == 1){

            // compute u
            for(int i = 0; i < ssp; ++i){
                u[i] = 0;
                for (int j = 0; j < length; ++j){
                    int index = i*length + j;
                    if(s[index]){
                        u[i] = u[i] ^ data[j];
                    }
                }
            }

            block * tmpM[nP+1];
            for(int i = 2; i <=nP;++i){
                tmpM[i] = new block[ssp];
            }

            // compute tmpM
            for (int i = 2; i <= nP; ++i){
                for (int j = 0; j < ssp; ++j){
                    tmpM[i][j] = zero_block;
                    for(int k = 0; k < length; ++k){
                        int index = j*length + k;
                        if(s[index]){
                            tmpM[i][j] = tmpM[i][j] ^ output[i][k];
                        }

                    }
                }
            }

            // send to the verifiers
            vector<future<void>> res;
			for (int i = 2; i<=nP; ++i){
				int party2 = i;
				res.push_back(pool->enqueue([this, u, tmpM, party2]() {
					io->send_data(party2, u, ssp*sizeof(bool));
					io->send_data(party2, tmpM[party2], ssp*sizeof(block));
				}));
			}
			joinNclean(res);

            for(int i = 2; i <=nP;++i){
                delete[] tmpM[i];
            }
        }
        else{
            block * tmpK = new block[ssp];
            block * tmpM = new block[ssp];

            // compute tmpK for party 1
			for(int i = 0; i <ssp; ++i){
				tmpK[i] = zero_block;
				for(int j = 0; j < length; ++j){
					int index = i*length + j;
					if (s[index]){
						tmpK[i] = tmpK[i] ^ output[1][j];
					}	
				}
			}

            io->recv_data(1, u, ssp*sizeof(bool));
			io->recv_data(1, tmpM, ssp*sizeof(block));

            // check the MAC
            bool flag = true;
            block tmp;
            for(int i = 0; i < ssp; ++i){
                tmp = tmpM[i];
                if(u[i])
                    tmp = tmp ^ Delta;
                if(!cmpBlock(&tmp, &tmpK[i], 1))
                    flag = false;
            }
            if (!flag)
                cout << "Consistency checks for mv-svole fail" << endl;
        }

    }

    /* debug function*/
    void debug_check(block * output[nP+1], bool* data, int length){
        if(party == 1){
            for(int i = 2; i <= nP; ++i){
                io->send_data(i, output[i], length*sizeof(block));
                io->send_data(i, data, length*sizeof(bool));
            }
        }
        else{
            block * recv_mac = new block[length];
            io->recv_data(1, recv_mac, length*sizeof(block));
            io->recv_data(1, data, length*sizeof(bool));

            for(int i = 0; i < length; ++i){
                block tmp = recv_mac[i];
                if(data[i])
                    tmp = tmp ^ Delta;
                if(!cmpBlock(&tmp, &output[1][i], 1))
                    cout << "sVOLE fails!" << endl;
            }
        }
    }
	
};
