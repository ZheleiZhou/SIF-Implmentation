#pragma once

#include <emp-tool/emp-tool.h>
#include "emp-agmpc/emp-agmpc.h"
#include "sif/test_sif.h"

const string circuits_location = string("../circuits/bristol_fashion/");
using namespace std;
using namespace emp;

const static int nP = 3;
int party, port;
int main(int argc, char** argv) {
	parse_party_and_port(argv, &party, &port);
	if(party > nP)return 0;
	NetIOMP<nP> io(party, port);
#ifdef LOCALHOST
	NetIOMP<nP> io2(party, port+2*(nP+1)*(nP+1)+1);
#else
	NetIOMP<nP> io2(party, port+2*(nP+1));
#endif
	NetIOMP<nP> *ios[2] = {&io, &io2};
	ThreadPool pool(2*(nP-1)+2);	

	if (party == 1){
		cout << "Evaluate Boolean Circuit" << endl;
	}
	sif_2r_bool_bench_once<nP>(party, ios, &pool, circuits_location+"aes_128.txt");

	return 0;
}