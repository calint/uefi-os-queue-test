#pragma once

#include <atomic>
#include <cstdint>

struct Job {
    uint64_t payload;
    std::atomic<uint64_t>* counter;

    void run() {
        // Simulate "heavy" arithmetic intensity
        uint64_t val = payload;
        for (int i = 0; i < 1'000'000; ++i) {
            val = ((val << 5) + val) + i; // simple hash-like work
        }

        // Tells the compiler 'val' is used here, don't optimize it away
        asm volatile("" : : "g"(val) : "memory");

        counter->fetch_add(1, std::memory_order_relaxed);
    }
};
