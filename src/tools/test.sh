#!/bin/bash

rm -rf out.bsp
make
./bsp/bsp ../../modelsrc/bsp/bsp_test4_areahints.txt
./dumpbsp/dumpbsp out.bsp
