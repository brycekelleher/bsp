#!/bin/bash

rm -rf out.bsp
make
./bsp/bsp ../../modelsrc/bsp/bsp_test4_areahints.txt
#./bsp/bsp ../../modesrc/bsp/bsp_test2.txt
./dumpbsp/dumpbsp out.bsp
