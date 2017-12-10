#!/bin/bash
module load libraries/openmpi-2.0.1-gcc-5.4.0
mpirun -n 2 ./server
