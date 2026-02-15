#pragma once

#include <atomic>
#include <cstdint>

auto constexpr iterations = 1'000'000u;

struct Job {
    uint64_t payload;
    std::atomic<uint64_t>* counter;

    void run() {
        // simulate "heavy" arithmetic intensity
        auto val = payload;
        for (auto i = 0u; i < iterations; ++i) {
            val = ((val << 5) + val) + i; // simple hash-like work
        }

        // tells the compiler 'val' is used here, don't optimize it away
        asm volatile("" : : "g"(val) : "memory");

        counter->fetch_add(1, std::memory_order_relaxed);
    }
};
