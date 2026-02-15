#include "osca.hpp"
#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>
#include <vector>

// A "Heavy" Job simulation
struct BenchmarkJob {
    uint64_t payload;
    std::atomic<uint64_t>* counter;

    void run() {
        // Simulate "heavy" arithmetic intensity
        uint64_t val = payload;
        for (int i = 0; i < 1'000'000; ++i) {
            val = ((val << 5) + val) + i; // simple hash-like work
        }
        counter->fetch_add(1, std::memory_order_relaxed);
    }
};

void run_stress_test(uint32_t num_consumers, uint32_t total_jobs) {
    std::atomic<uint64_t> completed_jobs{0};

    // 1. Start Consumers
    std::vector<std::jthread> consumers;
    for (uint32_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([](std::stop_token stoken) {
            while (!stoken.stop_requested()) {
                if (!osca::jobs.run_next()) {
                    __builtin_ia32_pause();
                }
            }
        });
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 2. Producer: Flood the queue
    for (uint32_t i = 0; i < total_jobs; ++i) {
        // Uses the blocking add() to wait if queue is full
        osca::jobs.add<BenchmarkJob>(uint64_t(i), &completed_jobs);
    }

    // 3. Synchronization Point
    osca::jobs.wait_idle();

    auto end_time = std::chrono::high_resolution_clock::now();

    // 4. Join threads to verify termination
    for (auto& t : consumers) {
        t.request_stop();
    }

    // Stats
    std::chrono::duration<double> diff = end_time - start_time;
    double m_jobs_per_sec = (total_jobs / diff.count()) / 1'000'000.0;

    std::cout << "--- Stress Test Results ---" << std::endl;
    std::cout << "Consumers:    " << num_consumers << std::endl;
    std::cout << "Total Jobs:   " << total_jobs << std::endl;
    std::cout << "Elapsed Time: " << diff.count() << " s" << std::endl;
    std::cout << "Throughput:   " << m_jobs_per_sec << " Million Jobs/sec"
              << std::endl;
    std::cout << "Verified:     " << completed_jobs.load() << " / "
              << total_jobs << std::endl;
}

int main(int argc, char** argv) {
    uint32_t consumers = (argc > 1) ? std::stoi(argv[1]) : 1;
    uint32_t jobs = (argc > 2) ? std::stoi(argv[2]) : 100'00;
    std::cout << "Consumers: " << consumers << std::endl;
    std::cout << "Jobs:      " << jobs << std::endl;
    osca::jobs.init();
    run_stress_test(consumers, jobs);
    return 0;
}
