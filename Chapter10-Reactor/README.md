Chapter 10 — External Integration Without Architectural Drift

This repository accompanies Chapter 10 of [Designing Deterministic Systems with Modern C++](https://leanpub.com/buildingembeddedsystemsonlinuxamoderncapproach).

In this chapter, the system built in Chapter 9 is extended to integrate real external inputs (UDP and UART) using epoll, while preserving the deterministic structure established earlier.

Key properties maintained:

The System Controller still owns all threads.

The reactor is a multiplexer, not a scheduler.

External uncertainty is contained inside SensorContext.

No hidden threads.

No dynamic topology.

Bounded queues remain the only concurrency boundaries.

Measurements remain value types.

The goal is not to build an I/O framework.

The goal is to demonstrate how to integrate unpredictable external inputs without allowing concurrency, allocation, or policy to leak across architectural boundaries.

The full design rationale, tradeoffs, and progression from Chapter 9 are explained in the book.
