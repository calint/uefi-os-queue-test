#include "kernel.hpp"
#include "osca.hpp"
#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>
#include <vector>

#include "test.hpp"

void run_test(uint32_t num_consumers, uint32_t total_jobs) {
    std::atomic<uint64_t> completed_jobs{0};

    // start consumers
    std::vector<std::jthread> consumers;
    for (auto i = 0u; i < num_consumers; ++i) {
        consumers.emplace_back([](std::stop_token stoken) {
            while (!stoken.stop_requested()) {
                if (!osca::jobs.run_next()) {
                    kernel::core::pause();
                }
            }
        });
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // producer: flood the queue
    for (auto i = 0u; i < total_jobs; ++i) {
        osca::jobs.add<Job>(uint64_t(i), &completed_jobs);
    }

    osca::jobs.wait_idle();

    auto end_time = std::chrono::high_resolution_clock::now();

    for (auto& t : consumers) {
        t.request_stop();
    }

    std::chrono::duration<double> diff = end_time - start_time;

    std::cout << "Results for 1P / " << num_consumers << "C:\n";
    std::cout << "      Time: " << diff.count() << " s" << "\n";
    std::cout << "Throughput: " << (total_jobs / diff.count()) << " jobs/sec\n";
    std::cout << "  Verified: " << completed_jobs.load() << " / " << total_jobs
              << "\n\n";
}

int main(int argc, char** argv) {
    uint32_t consumers = (argc > 1) ? std::stoi(argv[1]) : 1;
    uint32_t jobs = (argc > 2) ? std::stoi(argv[2]) : 10'000;
    std::cout << "Consumers: " << consumers << "\n";
    std::cout << "     Jobs: " << jobs << "\n\n";

    osca::jobs.init();

    run_test(consumers, jobs);
}
