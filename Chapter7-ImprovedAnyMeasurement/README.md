Chapter 7 – AnyMeasurement (Final Snapshot)

This repository accompanies Chapter 7 of _Designing Deterministic Systems with Modern C++_.

This archive is a clean snapshot intended for the Chapter 7 walkthrough.

Guarantees:
- No C++ exceptions are used anywhere in this snapshot
- Fixed-size SBO storage
- Manual ops table ("minimal v-table")
- Separate BDS and NMEA serialization hooks
- get<T>() uses assert; try_get<T>() returns nullptr

Sanity check command:
  grep -R -n -E "\\bthrow\\b|runtime_error|logic_error|bad_cast" .
