#!/bin/bash

make
./bsp/bsp ../../modelsrc/bsp/bsp_test4_areahints.txt
./dumpbsp/dumpbsp out.bsp
