# Chapter 6 - ASCII Data Serialization with NMEA (Iterator Tokenization, No Framer)

This variant focuses on tokenization + extraction only.

- Tokenizer performs structural checks, identifier extraction, checksum verification,
  and computes `fieldCount()`.
- Fields are produced on-demand via a forward iterator that yields `std::string_view`.
- No list of field views is stored (no `std::vector<std::string_view>`).
- No sentence framing helper is included.

## Build

```sh
mkdir -p build
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/nmea_demo
```
