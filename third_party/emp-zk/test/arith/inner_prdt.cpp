#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>

using namespace emp;
using namespace std;

int port, party;
int repeat, sz;
const int threads = 1;

void test_inner_product(BoolIO<NetIO> *ios[threads], int party) {
  srand(time(NULL));
  uint64_t constant = 0;
  uint64_t *witness = new uint64_t[2 * sz];
  memset(witness, 0, 2 * sz * sizeof(uint64_t));

  setup_zk_arith<BoolIO<NetIO>>(ios, threads, party);

  IntFp *x = new IntFp[2 * sz];

  if (party == ALICE) {
    uint64_t sum = 0, tmp;
    for (int i = 0; i < sz; ++i) {
      witness[i] = rand() % PR;
      witness[sz + i] = rand() % PR;
    }
    for (int i = 0; i < sz; ++i) {
      tmp = mult_mod(witness[i], witness[sz + i]);
      sum = add_mod(sum, tmp);
    }
    constant = PR - sum;
    ios[0]->send_data(&constant, sizeof(uint64_t));
  } else {
    ios[0]->recv_data(&constant, sizeof(uint64_t));
  }

  for (int i = 0; i < 2 * sz; ++i)
    x[i] = IntFp(witness[i], ALICE);

  auto start = clock_start();
  for (int j = 0; j < repeat; ++j) {
    fp_zkp_inner_prdt<BoolIO<NetIO>>(x, x + sz, constant, sz);
  }

  finalize_zk_arith<BoolIO<NetIO>>();

  double tt = time_from(start);
  cout << "prove " << repeat << " degree-2 polynomial of length " << sz << endl;
  cout << "time use: " << tt / 1000 << " ms" << endl;
  cout << "average time use: " << tt / 1000 / repeat << " ms" << endl;

  delete[] witness;
  delete[] x;
}

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  BoolIO<NetIO> *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new BoolIO<NetIO>(
        new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i),
        party == ALICE);

  std::cout << std::endl << "------------ ";
  std::cout << "ZKP inner product test";
  std::cout << " ------------" << std::endl << std::endl;
  ;

  if (argc < 3) {
    std::cout << "usage: [binary] PARTY PORT POLY_NUM POLY_DIMENSION"
              << std::endl;
    return -1;
  } else if (argc < 5) {
    repeat = 100;
    sz = 10;
  } else {
    repeat = atoi(argv[3]);
    sz = atoi(argv[4]);
  }

  test_inner_product(ios, party);

  for (int i = 0; i < threads; ++i) {
    delete ios[i]->io;
    delete ios[i];
  }
  return 0;
}
