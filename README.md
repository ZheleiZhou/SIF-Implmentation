# Single-Input Functionalities

This is a prototype implementation of 1-round and 2-round Single-Input Functionalities (SIF). SIF is a special case of Multi-Party Computation (MPC); in MPC, each party have a secret input while in SIF, only a special party called dealer has a secret input.

## Installation

We implement our SIF protocols on a machine running Ubuntu 22.04 LTS. You can use the following commands to install the packages that will be used:

```
sudo apt-get update
sudo apt-get install -y cmake git build-essential
```

## Running a simple test

First, you need to use the following commands to build the executables:
```
mkdir build
cd build
cmake ..
make -j4
```

If you wish to run our 1-round SIF protocol over binary field on a AES-128 circuit for 3 parties, you can use the following command:
```
../script/run_3 ./test/aes_iknp_test 
```

## Running with different number of parties

If you want to run with different number of parties, you need to reset the value of the party number variable, which is set as nP. For instance, if you want to run our 1-round SIF protocol over binary field on a AES-128 circuit for 5 parties, you have to reset the nP value to be 5 in aes_iknp.cc, which is put in the test folder.

If you want to run the protocol instances with different machines or with more than 40 parties, you have to change the corresponding IP addresses or the maximum permissible party number in cmpc_config.h, which is put in the third_party/emp-agmpc folder.