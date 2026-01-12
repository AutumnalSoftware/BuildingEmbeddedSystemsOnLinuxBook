# AnyMeasurementChapter7 (Header-Only `AnyMeasurement`)

Book-friendly layout:

- `AnyMeasurement` implementation is header-only:
  - `include/AnyMeasurement/AnyMeasurement.h`
- Measurement enums/payload structs/header are declared in:
  - `include/AnyMeasurement/MeasurementTypes.h`
- String conversions / stream operators for enums + header are defined in:
  - `src/MeasurementTypes.cpp`

**Note:** Serialization here is intentionally simple (binary POD writes) for Chapter 7.
It is a placeholder for your book's stream/serialization layer.

## Build

```bash
mkdir -p build
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run demo

```bash
./build/anymeasurement_demo
```
