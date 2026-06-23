#pragma once

#include <atomic>
#include <new>


// A counter that is isolated on a cache line.  It isn't thread-safe - it must be accessed only
// on one thread at a time - but on that thread it won't cause a cache line to be sloshed around
// from processor to processor.
struct Counter
{
  void incr() { ++value; }
  void operator++() { incr(); }
  void operator++(int) { incr(); }

  operator unsigned long() const { return value.load(); }

private:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
  static constexpr auto cache_line_size = std::hardware_destructive_interference_size;
#pragma GCC diagnostic pop
  alignas(cache_line_size) std::atomic<unsigned long> value {0};
  char _padding_[cache_line_size - sizeof(long)];
};
