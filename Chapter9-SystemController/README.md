// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

# Chapter 9 - System Controller (Snapshot)

This Chapter 9 demo is about system composition and control.

- Uses `std::thread`
- No Boost.Asio
- No real I/O
- Test threads generate measurements
- Concurrency boundaries are explicit bounded queues (MoodyCamel SPSC)

## Dependencies

This snapshot assumes the repository already provides:

- `ThirdParty/readerwriterqueue`
- `ThirdParty/boost_asio_1_36_0` (not used in Chapter 9, but present for later chapters)
- `../Chapter7-ImprovedAnyMeasurement/include` containing:
  - `MeasurementTypes.h`
  - `AnyMeasurement.h`
  - `MeasurementTypesIO.h`

No third-party sources are duplicated in this snapshot.

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j
./Chapter9_SystemController
```

