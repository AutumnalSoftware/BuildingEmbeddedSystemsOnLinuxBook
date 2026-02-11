# Chapter 10 — External Integration

This repository accompanies Chapter 10 of [Designing Deterministic Systems with Modern C++](https://leanpub.com/buildingembeddedsystemsonlinuxamoderncapproach).

In this chapter, the system built in Chapter 9 is extended to integrate real external inputs (UDP and UART) using epoll, while preserving the deterministic structure established earlier.

Key properties maintained:

* The System Controller still owns all threads.

* The reactor is a multiplexer, not a scheduler.

* External uncertainty is contained inside _SensorContext_.

* No hidden threads.

* No dynamic topology.

* Bounded queues remain the only concurrency boundaries.

* Measurements are value types.

The goal is not to build an I/O framework.

The goal is to demonstrate how to integrate unpredictable external inputs without allowing concurrency, allocation, or policy to leak across architectural boundaries.

The full design rationale, tradeoffs, and progression from Chapter 10 are explained in the book.

# Building

``` cmake -S . -B build
``` cmake --build build

# Running the System

In one terminal:

./build/your_binary_name

The reactor will start and wait for external input.

Simulating UDP Input (Packet-Oriented)

In a second terminal:

echo '$GPGLL,4916.45,N,12311.12,W,225444,A*1D' | nc -u -w1 127.0.0.1 9000


Or continuously:

while true; do
  echo '$GPGLL,4916.45,N,12311.12,W,225444,A*1D'
  sleep 1
done | nc -u -w1 127.0.0.1 9000


(Replace port if needed.)

Simulating UART via socat (Stream-Oriented)

Create a pseudo-terminal pair:

socat -d -d pty,raw,echo=0 pty,raw,echo=0


It will print two PTY paths, for example:

/dev/pts/3
/dev/pts/4


Configure the system to open one side (e.g., /dev/pts/3).

In another terminal, write to the other side:

echo '$GPGLL,4916.45,N,12311.12,W,225444,A*1D' > /dev/pts/4


This simulates a UART delivering stream input.

Why This Matters
