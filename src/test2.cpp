#include "osca.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stop_token>
#include <thread>
#include <vector>

#include "test.hpp"

void run_test(uint32_t producers, uint32_t consumers, uint32_t jobs,
              uint64_t job_work) {

    std::atomic<uint64_t> completed_jobs{0};

    auto start = std::chrono::high_resolution_clock::now();

    // launch consumers
    std::vector<std::jthread> consumer_threads;
    for (auto i = 0u; i < consumers; ++i) {
        consumer_threads.emplace_back([&](std::stop_token st) {
            while (!st.stop_requested()) {
                if (!osca::jobs.run_next()) {
                    kernel::core::pause();
                }
            }
        });
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // launch producers
    std::vector<std::jthread> producer_threads;
    auto jobs_per_producer = jobs / producers;
    for (auto i = 0u; i < producers; ++i) {
        producer_threads.emplace_back([&] {
            for (auto j = 0u; j < jobs_per_producer; ++j) {
                while (!osca::jobs.try_add<Job>(j, job_work, &completed_jobs)) {
                    kernel::core::pause();
                }
            }
        });
    }

    // wait for all work to finish
    for (auto& p : producer_threads)
        p.join();

    osca::jobs.wait_idle();

    auto end_time = std::chrono::high_resolution_clock::now();

    for (auto& c : consumer_threads)
        c.request_stop();

    std::chrono::duration<double> diff = end_time - start_time;

    std::cout << "Results for " << producers << "P / " << consumers << "C:\n";
    std::cout << "      Time: " << diff.count() << " s" << std::endl;
    std::cout << "Throughput: " << (jobs / diff.count()) << " jobs/sec\n";
    std::cout << "  Verified: " << completed_jobs.load() << " / " << jobs
              << "\n\n";
}

int main(int argc, char* argv[]) {
    uint32_t producers = (argc > 1) ? std::stoi(argv[1]) : 1;
    uint32_t consumers = (argc > 2) ? std::stoi(argv[2]) : 1;
    uint32_t jobs = (argc > 3) ? std::stoi(argv[3]) : 10'000;
    uint32_t job_work = (argc > 4) ? std::stoi(argv[4]) : 1'000'000;

    std::cout << "Producers: " << producers << "\n";
    std::cout << "Consumers: " << consumers << "\n";
    std::cout << "     Jobs: " << jobs << "\n";
    std::cout << " Job work: " << job_work << "\n\n";

    osca::jobs.init();

    run_test(producers, consumers, jobs, job_work);
}
