// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

# Chapter 8 - System Composition And Control (Snapshot)

This repository accompanies Chapter 8 of _Designing Deterministic Systems with Modern C++_.

This Chapter 8 demo is about system composition and control.

- Uses `std::thread`
- No Boost.Asio
- No real I/O
- Test threads generate measurements
- Concurrency boundaries are explicit bounded queues (MoodyCamel SPSC)

## Dependencies

This snapshot assumes the repository already provides:

- `ThirdParty/readerwriterqueue`
- `ThirdParty/boost_asio_1_36_0` (not used in Chapter 8, but present for later chapters)
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

