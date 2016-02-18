#!/bin/bash

BSP=./bsp/bsp
BSPDUMP=./bspdump/bspdump

rm -rf out.bsp
$BSP ./bsp/bsp_test0.txt 
xxd -c 4 -g 4 out.bsp > out.bsp.hexdump
$BSPDUMP out.bsp > out.bsp.dump
