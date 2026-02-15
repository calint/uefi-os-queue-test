#include "osca.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stop_token>
#include <thread>
#include <vector>

using namespace osca;

// 1. Define a job that fits the 48-byte capture limit
struct BenchmarkJob {
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

void run_benchmark(uint32_t num_producers, uint32_t num_consumers,
                   uint32_t total_jobs) {

    std::atomic<uint64_t> completed_jobs{0};

    auto start = std::chrono::high_resolution_clock::now();

    // 2. Launch Consumers
    std::vector<std::jthread> consumers;
    for (uint32_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&](std::stop_token st) {
            while (!st.stop_requested()) {
                if (!jobs.run_next()) {
                    kernel::core::pause(); // Avoid burning CPU if empty
                }
            }
        });
    }

    // 3. Launch Producers
    std::vector<std::jthread> producers;
    uint32_t jobs_per_producer = total_jobs / num_producers;
    for (uint32_t i = 0; i < num_producers; ++i) {
        producers.emplace_back([&] {
            for (uint32_t j = 0; j < jobs_per_producer; ++j) {
                // Use C++26 standard keywords for logic as requested
                while (!jobs.try_add<BenchmarkJob>(j, &completed_jobs)) {
                    kernel::core::pause();
                }
            }
        });
    }

    // 4. Wait for all work to finish
    for (auto& p : producers)
        p.join();
    jobs.wait_idle();
    for (auto& c : consumers)
        c.request_stop();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Results for " << num_producers << "P / " << num_consumers
              << "C:\n";
    std::cout << "  Throughput: " << (total_jobs / elapsed.count())
              << " ops/sec\n";
    std::cout << "  Verified: " << completed_jobs.load() << " / " << total_jobs
              << "\n";
}

int main(int argc, char* argv[]) {
    jobs.init();

    // Default values
    uint32_t producers = 4;
    uint32_t consumers = 4;
    uint32_t num_jobs = 100'000;

    if (argc > 1)
        producers = std::stoul(argv[1]);
    if (argc > 2)
        consumers = std::stoul(argv[2]);
    if (argc > 3)
        num_jobs = std::stoul(argv[3]);

    // Basic validation for the total jobs distribution
    if (producers == 0 || consumers == 0) {
        std::cerr << "Error: Producers and consumers must be > 0\n";
        return 1;
    }

    std::cout << "Starting benchmark: " << producers << " producers, "
              << consumers << " consumers, " << num_jobs << " jobs...\n";

    run_benchmark(producers, consumers, num_jobs);

    return 0;
}
