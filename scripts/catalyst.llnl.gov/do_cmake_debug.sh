#!/bin/bash


INSTALL_PREFIX=${PWD}


rm CMakeCache.txt

cmake ../../ \
  -DHAVOQGT_BUILD_TEST=TRUE \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMPI_C_COMPILER=/usr/tce/packages/openmpi/openmpi-2.0.0-gcc-4.9.3/bin/mpicc \
  -DMPI_CXX_COMPILER=/usr/tce/packages/openmpi/openmpi-2.0.0-gcc-4.9.3/bin/mpicxx \
  -DBOOST_ROOT=/usr/gapps/dst/opt/boost_stages/boost_1_56_0/ \
  -DCMAKE_CXX_FLAGS="-std=c++11 -Wredundant-decls -DDEBUG -DDEBUG_DPG -lrt -lpthread -lm" \


