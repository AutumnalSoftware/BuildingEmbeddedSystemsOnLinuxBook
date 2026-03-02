# Chapter 12 - Deterministic Fusion and Output (UDP ingest milestone)

This chapter starts the "meaning" layer by first making external inputs real.

Milestone implemented here:

- Receive **binary BDS measurements over UDP**
- Deserialize using `CommonBdsMeasurementCodecs`
- Wrap into `AnyMeasurement` and enqueue into the system's bounded queue

Fusion timing logic (sync windows, match logging) will be layered on next.

## Build

From the repository root, this directory is expected to live alongside `Common/` and `ThirdParty/`.

```bash
mkdir -p build/ch12
cd build/ch12
cmake ../../Chapter12-FusionTiming
cmake --build . -j
```

## Run

Terminal 1 (receiver):

```bash
./chapter12_input_fusion_timing
```

Terminal 2 (transmitter):

```bash
./weather_tx --udp 127.0.0.1:9000 --temp-hz 10 --duration 0
```

You should see the Chapter 12 stats line report temperature dequeues.
