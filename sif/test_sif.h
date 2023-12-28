#pragma once

#include "1r_sif_bool_pcg.h"
#include "1r_sif_bool_iknp.h"
#include "1r_sif_arith.h"
#include "2r_sif_bool.h"
#include "test/test.h"

using namespace std;

template<int nP>
void sif_1r_bool_pcg_bench_once(int party, NetIOMP<nP> * ios[2], ThreadPool * pool, string filename) {
	if(party == 1)cout <<"CIRCUIT:\t"<<filename<<endl;
	//string file = circuit_file_location+"/"+filename;
	BristolFashion cf(filename.c_str());

	auto start = clock_start();
	OneRound_SIF_Bool_PCG<nP>* sif = new OneRound_SIF_Bool_PCG<nP>(ios, pool, party, &cf);
	double t2 = time_from(start);
	cout << "party " << party << ": setup time " <<  t2/1000.0 <<"ms\n"<<flush;
	ios[0]->sync();
	ios[1]->sync();

	start = clock_start();
	sif->Preprocess();
	t2 =  time_from(start);
	cout << "party " << party << ": preprocessing time " <<  t2/1000.0 <<"ms\n"<<flush;
	ios[0]->flush();
	ios[1]->flush();

	bool *in = new bool[cf.num_input]; bool *out = new bool[cf.num_output];
	memset(in, 0, cf.num_input*sizeof(bool));
	start = clock_start();
	sif->online(in, out);
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);
	cout << "party " << party << ": online time " <<  t2/1000.0 <<"ms\n"<<flush;
	delete sif;
}

template<int nP>
void sif_1r_bool_iknp_bench_once(int party, NetIOMP<nP> * ios[2], ThreadPool * pool, string filename) {
	if(party == 1)cout <<"CIRCUIT:\t"<<filename<<endl;
	//string file = circuit_file_location+"/"+filename;
	BristolFashion cf(filename.c_str());

	auto start = clock_start();
	OneRound_SIF_Bool_IKNP<nP>* sif = new OneRound_SIF_Bool_IKNP<nP>(ios, pool, party, &cf);
	double t2 = time_from(start);
	cout << "party " << party << ": setup time " <<  t2/1000.0 <<"ms\n"<<flush;
	ios[0]->sync();
	ios[1]->sync();

	start = clock_start();
	sif->Preprocess();
	t2 =  time_from(start);
	cout << "party " << party << ": preprocessing time " <<  t2/1000.0 <<"ms\n"<<flush;
	ios[0]->flush();
	ios[1]->flush();

	bool *in = new bool[cf.num_input]; bool *out = new bool[cf.num_output];
	memset(in, 0, cf.num_input*sizeof(bool));
	start = clock_start();
	sif->online(in, out);
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);
	cout << "party " << party << ": online time " <<  t2/1000.0 <<"ms\n"<<flush;
	delete sif;
}

template<int nP>
void sif_2r_bool_bench_once(int party, NetIOMP<nP> * ios[2], ThreadPool * pool, string filename) {
	if(party == 1)cout <<"CIRCUIT:\t"<<filename<<endl;
	//string file = circuit_file_location+"/"+filename;
	BristolFashion cf(filename.c_str());

	auto start = clock_start();
	TwoRound_SIF_Bool<nP>* sif = new TwoRound_SIF_Bool<nP>(ios, pool, party, &cf);
	double t2 = time_from(start);
	cout << "party " << party << ": setup time " <<  t2/1000.0 <<"ms\n"<<flush;
	ios[0]->sync();
	ios[1]->sync();

	start = clock_start();
	sif->Preprocess();
	t2 =  time_from(start);
	cout << "party " << party << ": preprocessing time " <<  t2/1000.0 <<"ms\n"<<flush;
	ios[0]->flush();
	ios[1]->flush();

	bool *in = new bool[cf.num_input]; bool *out = new bool[cf.num_output];
	memset(in, 0, cf.num_input*sizeof(bool));
	start = clock_start();
	sif->online(in, out);
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);
	cout << "party " << party << ": online time " <<  t2/1000.0 <<"ms\n"<<flush;
	delete sif;
}

template<int nP>
void sif_1r_arith_bench_once(int party, NetIOMP<nP> * ios[2], ThreadPool * pool, string filename) {
	if(party == 1)cout <<"CIRCUIT:\t"<<filename<<endl;
	//string file = circuit_file_location+"/"+filename;
	BristolFashion cf(filename.c_str());

	auto start = clock_start();
	OneRound_SIF_Arith<nP>* sif = new OneRound_SIF_Arith<nP>(ios, pool, party, &cf);
	double t2 = time_from(start);
	cout << "party " << party << ": setup time " <<  t2/1000.0 <<"ms\n"<<flush;
	ios[0]->sync();
	ios[1]->sync();

	start = clock_start();
	sif->Preprocess();
	t2 =  time_from(start);
	cout << "party " << party << ": preprocessing time " <<  t2/1000.0 <<"ms\n"<<flush;
	ios[0]->flush();
	ios[1]->flush();

	uint64_t *in = new uint64_t[cf.num_input]; uint64_t *out = new uint64_t[cf.num_output];
	memset(in, 0, cf.num_input*sizeof(uint64_t));
	start = clock_start();
	sif->online(in, out);
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);
	cout << "party " << party << ": online time " <<  t2/1000.0 <<"ms\n"<<flush;
	delete sif;
}