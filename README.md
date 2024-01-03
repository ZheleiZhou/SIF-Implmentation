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
make
```

If you wish to run our 1-round SIF protocol over binary field on a AES-128 circuit for 3 parties, you can use the following command:
```
../script/run_3 ./test/aes_iknp_test 
```
