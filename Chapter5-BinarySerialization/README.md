# Chapter 5 – Binary Serialization

This directory contains the code and examples for **Chapter 5: Binary Serialization** from  
*Building Embedded Systems on Linux: A Modern C++ Approach*.

This chapter introduces a small, explicit, embedded-friendly approach to **binary serialization** using positional fields and a reader-makes-right design philosophy.

---

## Scope of This Chapter

Chapter 5 focuses **exclusively on binary serialization**.

Specifically, it covers:

- Positional, reader-makes-right binary formats
- Explicit message framing
- Deterministic buffering and bounds checking
- Endianness handling without scattering byte-swap logic
- Serialization logic separated from data structures
- No dynamic allocation
- No exceptions in the mainline code path

ASCII and NMEA serialization are introduced in the following chapter.

---

## BinaryDataStream (BDS)

The core of this chapter is `BinaryDataStream`, a small, header-only utility used throughout the rest of the book.

BinaryDataStream provides:

- Deterministic binary serialization and deserialization
- Explicit host vs wire endianness handling
- Framed messages using fixed-layout headers
- Bounded reads and writes via non-owning byte views
- A latched error model that prevents partial reads or writes

BinaryDataStream does **not**:

- Allocate memory
- Perform I/O
- Manage threads
- Describe schemas or perform reflection

Serialization is treated as a mechanical transformation from values to bytes and back.
