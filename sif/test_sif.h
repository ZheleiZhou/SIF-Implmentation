#pragma once

#include "1r_sif_bool_pcg.h"
#include "1r_sif_bool_iknp.h"
#include "1r_sif_arith.h"
#include "2r_sif_bool.h"

using namespace std;
using namespace emp;

template<int nP>
int communication(NetIOMP<nP> * ios[2]) {
	return ios[0]->count() + ios[1]->count();
}

template<int nP>
void sif_1r_bool_pcg_bench_once(int party, NetIOMP<nP> * ios[2], ThreadPool * pool, string filename) {
	BristolFashion cf(filename.c_str());

	auto start = clock_start();
	OneRound_SIF_Bool_PCG<nP>* sif = new OneRound_SIF_Bool_PCG<nP>(ios, pool, party, &cf);
	double t2 = time_from(start);
	ios[0]->sync();
	ios[1]->sync();

	start = clock_start();
	sif->Preprocess();
	t2 =  time_from(start);
	cout << "party " << party << "/ preprocessing time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	int off_comm = communication<nP>(ios);
	cout << "party " << party << "/ preprocessing comm / " <<  off_comm/1000.0/1000.0 <<" /MB\n"<<flush;
	ios[0]->flush();
	ios[1]->flush();

	bool *in = new bool[cf.num_input]; bool *out = new bool[cf.num_output];
	memset(in, 0, cf.num_input*sizeof(bool));
	start = clock_start();
	sif->online(in, out);
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);
	cout << "party " << party << "/ online time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	cout << "party " << party << "/ online comm / " <<(communication<nP>(ios)-off_comm)/1000.0/1000.0<<" /MB\n"<<flush;
	delete sif;
}

template<int nP>
void sif_1r_bool_iknp_bench_once(int party, NetIOMP<nP> * ios[2], ThreadPool * pool, string filename) {
	BristolFashion cf(filename.c_str());

	auto start = clock_start();
	OneRound_SIF_Bool_IKNP<nP>* sif = new OneRound_SIF_Bool_IKNP<nP>(ios, pool, party, &cf);
	double t2 = time_from(start);
	ios[0]->sync();
	ios[1]->sync();

	start = clock_start();
	sif->Preprocess();
	t2 =  time_from(start);
	cout << "party " << party << "/ preprocessing time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	int off_comm = communication<nP>(ios);
	cout << "party " << party << "/ preprocessing comm / " <<  off_comm/1000.0/1000.0 <<" /MB\n"<<flush;
	ios[0]->flush();
	ios[1]->flush();

	bool *in = new bool[cf.num_input]; bool *out = new bool[cf.num_output];
	memset(in, 0, cf.num_input*sizeof(bool));
	start = clock_start();
	sif->online(in, out);
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);
	cout << "party " << party << "/ online time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	cout << "party " << party << "/ online comm / " <<(communication<nP>(ios)-off_comm)/1000.0/1000.0<<" /MB\n"<<flush;
	delete sif;
}

template<int nP>
void sif_2r_bool_bench_once(int party, NetIOMP<nP> * ios[2], ThreadPool * pool, string filename) {
	BristolFashion cf(filename.c_str());

	auto start = clock_start();
	TwoRound_SIF_Bool<nP>* sif = new TwoRound_SIF_Bool<nP>(ios, pool, party, &cf);
	double t2 = time_from(start);
	ios[0]->sync();
	ios[1]->sync();

	start = clock_start();
	sif->Preprocess();
	t2 =  time_from(start);
	cout << "party " << party << "/ preprocessing time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	int off_comm = communication<nP>(ios);
	cout << "party " << party << "/ preprocessing comm / " <<  off_comm/1000.0/1000.0 <<" /MB\n"<<flush;
	ios[0]->flush();
	ios[1]->flush();

	bool *in = new bool[cf.num_input]; bool *out = new bool[cf.num_output];
	memset(in, 0, cf.num_input*sizeof(bool));
	start = clock_start();
	sif->online(in, out);
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);
	cout << "party " << party << "/ online time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	cout << "party " << party << "/ online comm / " <<(communication<nP>(ios)-off_comm)/1000.0/1000.0<<" /MB\n"<<flush;
	delete sif;
}

template<int nP>
void sif_1r_arith_bench_once(int party, NetIOMP<nP> * ios[2], ThreadPool * pool, string filename) {
	BristolFashion cf(filename.c_str());

	auto start = clock_start();
	OneRound_SIF_Arith<nP>* sif = new OneRound_SIF_Arith<nP>(ios, pool, party, &cf);
	double t2 = time_from(start);
	ios[0]->sync();
	ios[1]->sync();

	start = clock_start();
	sif->Preprocess();
	t2 =  time_from(start);
	cout << "party " << party << "/ preprocessing time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	int off_comm = communication<nP>(ios);
	cout << "party " << party << "/ preprocessing comm / " <<  off_comm/1000.0/1000.0 <<" /MB\n"<<flush;
	ios[0]->flush();
	ios[1]->flush();

	uint64_t *in = new uint64_t[cf.num_input]; uint64_t *out = new uint64_t[cf.num_output];
	memset(in, 0, cf.num_input*sizeof(uint64_t));
	start = clock_start();
	sif->online(in, out);
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);
	cout << "party " << party << "/ online time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	cout << "party " << party << "/ online comm / " <<(communication<nP>(ios)-off_comm)/1000.0/1000.0<<" /MB\n"<<flush;
	delete sif;
}

template<int nP>
void bench_mpc_once(int party, NetIOMP<nP> * ios[2], ThreadPool * pool, string filename) {
	BristolFormat cf(filename.c_str());

	auto start = clock_start();
	CMPC<nP>* mpc = new CMPC<nP>(ios, pool, party, &cf);
	ios[0]->flush();
	ios[1]->flush();
	double t2 = time_from(start);
	ios[0]->sync();
	ios[1]->sync();

	start = clock_start();
	mpc->function_independent();

	mpc->function_dependent();
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);
	if(party == 1) 
		cout << "party " << party << "/ preprocessing time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	int off_comm = communication<nP>(ios);
	if(party == 1) 
		cout << "party " << party << "/ preprocessing comm / " <<  off_comm/1000.0/1000.0 <<" /MB\n"<<flush;
	ios[0]->flush();
	ios[1]->flush();

	bool *in = new bool[cf.n1+cf.n2]; bool *out = new bool[cf.n3];
	memset(in, false, cf.n1+cf.n2);
	start = clock_start();
	mpc->online(in, out);
	ios[0]->flush();
	ios[1]->flush();
	t2 = time_from(start);

	if(party == 1) 
		cout << "party " << party << "/ online time / " <<  t2/1000.0 <<" /ms\n"<<flush;
	if(party == 1) 
		cout << "party " << party << "/ online comm / " <<(communication<nP>(ios)-off_comm)/1000.0/1000.0<<" /MB\n"<<flush;
	delete mpc;
}