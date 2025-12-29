# TypeErasureExample
This directory contains example code demonstrating type erasure using external polymorphism.

Two simple example NMEA data structs are defined and used to demonstrate putting type erased NMEA sentences into a vector.

To build:

mkdir build
cmake -S . -B build
cd build
make

Then:

./TypeErasureDemo

Output should be:

talker=GP type=POS checksumValid=true
talker=GP type=STS checksumValid=true
