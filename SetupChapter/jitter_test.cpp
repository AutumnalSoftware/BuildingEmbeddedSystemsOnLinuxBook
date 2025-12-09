#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <cstdlib>

using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <iterations> <period_us>\n";
        std::cerr << "Example: " << argv[0]
                  << " 10000 1000   # 10,000 iterations, 1000 us (1 ms) period\n";
        return 1;
    }

    const long long iterations = std::atoll(argv[1]);
    const long long period_us  = std::atoll(argv[2]);

    if (iterations <= 0 || period_us <= 0)
    {
        std::cerr << "iterations and period_us must be positive.\n";
        return 1;
    }

    const auto period = std::chrono::microseconds{period_us};

    std::vector<long long> jitter_ns;
    jitter_ns.reserve(static_cast<std::size_t>(iterations));

    const auto start = Clock::now();
    auto next_wakeup = start + period;

    // Warm-up sleep to let the scheduler settle a bit (optional)
    std::this_thread::sleep_for(10ms);

    for (long long i = 0; i < iterations; ++i)
    {
        std::this_thread::sleep_until(next_wakeup);

        const auto now = Clock::now();
        const auto diff = now - next_wakeup;

        const auto diff_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count();
        jitter_ns.push_back(diff_ns);

        next_wakeup += period;
    }

    if (jitter_ns.empty())
    {
        std::cerr << "No samples collected.\n";
        return 1;
    }

    // Basic stats
    const auto min_it = std::min_element(jitter_ns.begin(), jitter_ns.end());
    const auto max_it = std::max_element(jitter_ns.begin(), jitter_ns.end());

    long double sum_abs = 0.0L;
    std::vector<long long> abs_jitter;
    abs_jitter.reserve(jitter_ns.size());

    for (auto v : jitter_ns)
    {
        const long long a = (v < 0) ? -v : v;
        abs_jitter.push_back(a);
        sum_abs += static_cast<long double>(a);
    }

    const long double avg_abs = sum_abs / static_cast<long double>(abs_jitter.size());

    std::sort(abs_jitter.begin(), abs_jitter.end());
    const std::size_t idx_99 = static_cast<std::size_t>(
        (abs_jitter.size() - 1) * 0.99
    );
    const long long p99_abs = abs_jitter[idx_99];

    std::cout << "Jitter statistics (nanoseconds)\n";
    std::cout << "  Samples:           " << jitter_ns.size() << "\n";
    std::cout << "  Min jitter:        " << *min_it << " ns (negative = early)\n";
    std::cout << "  Max jitter:        " << *max_it << " ns (positive = late)\n";
    std::cout << "  Avg |jitter|:      " << static_cast<long long>(avg_abs) << " ns\n";
    std::cout << "  99th % |jitter|:   " << p99_abs << " ns\n";

    return 0;
}

