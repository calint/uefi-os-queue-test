#include "osca.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

// A "Heavy" Job simulation
struct WorkJob {
    uint64_t payload;

    // The job entry point required by osca::is_job
    void run() {
        // Simulate "heavy" arithmetic intensity
        uint64_t val = payload;
        for (int i = 0; i < 10'000'000; ++i) {
            val = ((val << 5) + val) + i; // simple hash-like work
        }
    }
};

void run_stress_test(uint32_t num_consumers, uint32_t total_jobs) {
    osca::Jobs<1024> queue;
    queue.init();

    std::atomic<bool> should_terminate{false};

    // 1. Start Consumers
    std::vector<std::thread> consumers;
    for (uint32_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&queue, &should_terminate]() {
            while (true) {
                if (!queue.run_next()) {
                    if (should_terminate.load(std::memory_order_relaxed)) {
                        return;
                    }
                    __builtin_ia32_pause();
                }
            }
        });
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 2. Producer: Flood the queue
    for (uint32_t i = 0; i < total_jobs; ++i) {
        // Uses the blocking add() to wait if queue is full
        queue.add<WorkJob>(static_cast<uint64_t>(i));
    }

    // 3. Synchronization Point
    queue.wait_idle();

    auto end_time = std::chrono::high_resolution_clock::now();

    should_terminate.store(true, std::memory_order_relaxed);

    // 4. Join threads to verify termination
    for (auto& t : consumers) {
        if (t.joinable()) {
            t.join();
        }
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
}

int main(int argc, char** argv) {
    uint32_t threads = (argc > 1) ? std::stoi(argv[1]) : 1;
    uint32_t jobs = (argc > 2) ? std::stoi(argv[2]) : 100'000;
    run_stress_test(threads, jobs);
    return 0;
}
