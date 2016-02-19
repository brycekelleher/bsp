#!/bin/bash

BSP=./bsp/bsp
BSPDUMP=./bspdump/bspdump
SRCFILE=$1

rm -rf out.bsp out.bsp.dump out.bsp.hexdump
$BSP $SRCFILE
xxd -c 4 -g 4 out.bsp > out.bsp.hexdump
$BSPDUMP out.bsp > out.bsp.dump
