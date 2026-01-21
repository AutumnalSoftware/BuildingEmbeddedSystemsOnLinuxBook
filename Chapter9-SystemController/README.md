# Chapter 9 System Composition Demo (3 threads)

This is a standalone mini-project that demonstrates the Chapter 9 architecture:

- No Boost.Asio
- No real I/O
- One producer thread generates multiple measurement types at different rates
- A consumer/dispatcher thread gathers mechanical instrumentation
- A logger thread prints periodic summaries
- Data crosses thread boundaries by value through explicit bounded queues

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./Chapter9SystemDemo
```

## ThirdParty: readerwriterqueue

The book uses moodycamel::ReaderWriterQueue (SPSC) under `ThirdParty/readerwriterqueue`.

This ZIP includes a small stand-in header at:

- `ThirdParty/readerwriterqueue/readerwriterqueue.h`

If you already vendored the real library, replace that header with the upstream version.
