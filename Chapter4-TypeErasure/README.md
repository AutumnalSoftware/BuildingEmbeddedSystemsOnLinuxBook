# TypeErasureExample

This directory contains a minimal example demonstrating **type erasure via external polymorphism**.

Two simple NMEA sentence data structures are defined and stored by value in a single
`std::vector` using a type-erased wrapper. The example shows how unrelated concrete
types can share common behavior without inheritance or virtual functions.

## Build

```bash
mkdir build
cmake -S . -B build
cmake --build build

# Run
./TypeErasureDemo

# Expected Output
talker=GP type=POS checksumValid=true
talker=GP type=STS checksumValid=true

