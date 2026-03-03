# Chapter 11 - Fusion Timing (UDP ingest milestone)

This chapter starts the "meaning" layer by first making external inputs real.

Milestone implemented here:

- Receive **binary BDS measurements over UDP**
- Deserialize using `CommonBdsMeasurementCodecs`
- Wrap into `AnyMeasurement` and enqueue into the system's bounded queue

Fusion timing logic (sync windows, match logging) will be layered on next.

## Build

From the repository root, this directory is expected to live alongside `Common/` and `ThirdParty/`.

```bash
mkdir -p build/ch11
cd build/ch11
cmake ../../Chapter11-FusionTiming
cmake --build . -j
```

## Run

Terminal 1 (receiver):

```bash
./chapter11_input_fusion_timing
```

Terminal 2 (transmitter):

```bash
./weather_tx --udp 127.0.0.1:9000 --temp-hz 10 --duration 0
```

You should see the Chapter 11 stats line report temperature dequeues.
