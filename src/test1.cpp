#include "kernel.hpp"
#include "osca.hpp"
#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>
#include <vector>

#include "test.hpp"

void run_stress_test(uint32_t num_consumers, uint32_t total_jobs) {
    std::atomic<uint64_t> completed_jobs{0};

    // 1. Start Consumers
    std::vector<std::jthread> consumers;
    for (uint32_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([](std::stop_token stoken) {
            while (!stoken.stop_requested()) {
                if (!osca::jobs.run_next()) {
                    kernel::core::pause();
                }
            }
        });
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 2. Producer: Flood the queue
    for (uint32_t i = 0; i < total_jobs; ++i) {
        // Uses the blocking add() to wait if queue is full
        osca::jobs.add<Job>(uint64_t(i), &completed_jobs);
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

    std::cout << "Results for 1P / " << num_consumers << "C:\n";
    std::cout << "      Jobs:   " << total_jobs << "\n";
    std::cout << "      Time: " << diff.count() << " s" << "\n";
    std::cout << "Throughput: " << (total_jobs / diff.count()) << " jobs/sec\n";
    std::cout << "  Verified: " << completed_jobs.load() << " / " << total_jobs
              << "\n";
}

int main(int argc, char** argv) {
    uint32_t consumers = (argc > 1) ? std::stoi(argv[1]) : 1;
    uint32_t jobs = (argc > 2) ? std::stoi(argv[2]) : 10'000;
    std::cout << "Consumers: " << consumers << "\n";
    std::cout << "     Jobs: " << jobs << "\n\n";

    osca::jobs.init();

    run_stress_test(consumers, jobs);
}
