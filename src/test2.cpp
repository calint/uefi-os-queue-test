#include "osca.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stop_token>
#include <thread>
#include <vector>

#include "test.hpp"

void run_stress_test(uint32_t num_producers, uint32_t num_consumers,
                     uint32_t total_jobs) {

    std::atomic<uint64_t> completed_jobs{0};

    auto start = std::chrono::high_resolution_clock::now();

    // 2. Launch Consumers
    std::vector<std::jthread> consumers;
    for (uint32_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&](std::stop_token st) {
            while (!st.stop_requested()) {
                if (!osca::jobs.run_next()) {
                    kernel::core::pause(); // Avoid burning CPU if empty
                }
            }
        });
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 3. Launch Producers
    std::vector<std::jthread> producers;
    uint32_t jobs_per_producer = total_jobs / num_producers;
    for (uint32_t i = 0; i < num_producers; ++i) {
        producers.emplace_back([&] {
            for (uint32_t j = 0; j < jobs_per_producer; ++j) {
                // Use C++26 standard keywords for logic as requested
                while (!osca::jobs.try_add<Job>(j, &completed_jobs)) {
                    kernel::core::pause();
                }
            }
        });
    }

    // 4. Wait for all work to finish
    for (auto& p : producers)
        p.join();

    osca::jobs.wait_idle();

    auto end_time = std::chrono::high_resolution_clock::now();

    for (auto& c : consumers)
        c.request_stop();

    std::chrono::duration<double> diff = end_time - start_time;

    std::cout << "Results for " << num_producers << "P / " << num_consumers
              << "C:\n";
    std::cout << "      Time: " << diff.count() << " s" << std::endl;
    std::cout << "Throughput: " << (total_jobs / diff.count()) << " jobs/sec\n";
    std::cout << "  Verified: " << completed_jobs.load() << " / " << total_jobs
              << "\n\n";
}

int main(int argc, char* argv[]) {
    uint32_t producers = (argc > 1) ? std::stoi(argv[1]) : 1;
    uint32_t consumers = (argc > 2) ? std::stoi(argv[2]) : 1;
    uint32_t jobs = (argc > 3) ? std::stoi(argv[3]) : 10'000;
    std::cout << "Producers: " << producers << "\n";
    std::cout << "Consumers: " << consumers << "\n";
    std::cout << "     Jobs: " << jobs << "\n\n";

    osca::jobs.init();

    run_stress_test(producers, consumers, jobs);
}
