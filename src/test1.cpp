#include "kernel.hpp"
#include "osca.hpp"
#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>
#include <vector>

#include "test.hpp"

void run_test(uint32_t consumers, uint32_t jobs, uint64_t job_work) {
    std::atomic<uint64_t> completed_jobs{0};

    // start consumers
    std::vector<std::jthread> consumer_threads;
    for (auto i = 0u; i < consumers; ++i) {
        consumer_threads.emplace_back([](std::stop_token stoken) {
            while (!stoken.stop_requested()) {
                if (!osca::jobs.run_next()) {
                    kernel::core::pause();
                }
            }
        });
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // producer: flood the queue
    for (auto i = 0u; i < jobs; ++i) {
        osca::jobs.add<Job>(uint64_t(i), job_work, &completed_jobs);
    }

    osca::jobs.wait_idle();

    auto end_time = std::chrono::high_resolution_clock::now();

    for (auto& t : consumer_threads) {
        t.request_stop();
    }

    std::chrono::duration<double> diff = end_time - start_time;

    std::cout << "Results for 1P / " << consumers << "C:\n";
    std::cout << "      Time: " << diff.count() << " s" << "\n";
    std::cout << "Throughput: " << (jobs / diff.count()) << " jobs/sec\n";
    std::cout << "  Verified: " << completed_jobs.load() << " / " << jobs
              << "\n\n";
}

int main(int argc, char** argv) {
    uint32_t consumers = (argc > 1) ? std::stoi(argv[1]) : 1;
    uint32_t jobs = (argc > 2) ? std::stoi(argv[2]) : 10'000;
    uint32_t job_work = (argc > 3) ? std::stoi(argv[3]) : 1'000'000;

    std::cout << "Consumers: " << consumers << "\n";
    std::cout << "     Jobs: " << jobs << "\n";
    std::cout << " Job work: " << job_work << "\n\n";

    osca::jobs.init();

    run_test(consumers, jobs, job_work);
}
