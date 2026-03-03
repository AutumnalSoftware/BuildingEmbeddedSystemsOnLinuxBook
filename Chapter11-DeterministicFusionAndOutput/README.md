Good. But that README is still describing a halfway milestone.

Chapter 12 is not “UDP ingest milestone.”
Chapter 12 is **Deterministic Fusion and Output**.

This is the payoff chapter. The README should reflect that.

Here is a clean rewrite aligned with what your system actually does now.

---

# Chapter 12 – Deterministic Fusion and Output

Chapter 12 completes the system arc by turning external measurements into meaningful fused output.

This chapter builds on:

* Chapter 9: System composition and control
* Chapter 10: Reactor and framing
* Chapter 11: Fusion timing policy and observability

Chapter 12 adds:

* A bounded fused-output queue
* A dedicated output sink thread
* Deterministic emission policy (emit on Position arrival)
* Observability for fused enqueue, dequeue, and drops

No new abstractions are introduced.
No concurrency model changes.
This chapter is consequence, not invention.

---

## What This Milestone Demonstrates

* Binary BDS measurements received over UDP
* Deserialization via `CommonBdsMeasurementCodecs`
* Wrapping into `AnyMeasurement`
* Bounded SPSC queue handoff
* Fusion timing policy applied deterministically
* Fused weather samples emitted at a controlled rate
* Separate output stage for printing
* Full per-stage observability

With:

* Temperature at 10 Hz
* Position at 1 Hz
* Fused output at 1 Hz (emit on Position)

---

## Architecture Overview

Application
→ System Controller
→ Consumer / Fusion Policy
→ Fused Output Queue
→ Output Sink Thread
→ stdout

Infrastructure remains below the system boundary:

UDP ingest → Reactor → Framing → BDS decode

Fusion policy remains isolated from infrastructure.

---

## Build

From the repository root:

```
mkdir -p build/ch12
cd build/ch12
cmake ../../Chapter12-DeterministicFusionAndOutput
cmake --build . -j
```

---

## Run

Terminal 1 (receiver):

```
./chapter12_deterministic_fusion_and_output
```

Terminal 2 (transmitter):

```
./weather_tx \
  --udp 127.0.0.1:9000 \
  --temp-hz 10 \
  --position-hz 1
```

---

## Expected Output

You should see:

* `[FUSED]` lines at approximately 1 Hz
* `[STAT]` lines every second showing:

  * enqueue and dequeue rates
  * queue depth and high-water mark
  * fusion match statistics
  * fused enqueue/dequeue rates
  * end-to-end latency

Example steady-state output:

```
[FUSED] pos=(42.00, -77.00, 150.00) temp=22.60C dt=30.56ms
[STAT t=2s] enq(temp=10/s pos=1/s) deq(temp=10/s pos=1/s)
drops=0 (+0/s)
fused(enq=1/s deq=1/s fdrops=0 (+0/s))
q(depth=0 hi=1)
fusion(match=1/s missWin=0/s noTemp=0/s noPos=0/s)
latMax=1.07ms
```

---

## Key Design Points

* Fusion emits only on Position arrival
* Output printing is isolated in its own thread
* No printing inside the consumer
* No dynamic topology
* No hidden threads
* All concurrency boundaries are explicit queues

This chapter demonstrates that deterministic structure produces deterministic behavior.
