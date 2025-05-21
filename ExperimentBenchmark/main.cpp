#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <numeric>
#include <stdexcept>
#include "graph_plotter.hpp"

class RandomBenchmark {
public:
    RandomBenchmark(size_t num_experiments, size_t chunk_size)
        : NUM_EXPERIMENTS(num_experiments),
          CHUNK_SIZE(chunk_size),
          buffer_(chunk_size) {}

    void run() {
        std::cout << "Starting benchmark with " << NUM_EXPERIMENTS 
                  << " iterations of " << CHUNK_SIZE/(1024*1024) << "MB reads\n";

        for (size_t i = 0; i < NUM_EXPERIMENTS; ++i) {
            auto duration = run_single_iteration(i);
            timings_.push_back(duration);
            print_iteration_stats(i, duration);
        }

        analyze_results();
        visualize_results();
    }

private:
    const size_t NUM_EXPERIMENTS;
    const size_t CHUNK_SIZE;
    std::vector<char> buffer_;
    std::vector<double> timings_;

    double run_single_iteration(size_t iteration) {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::ifstream random("/dev/random", std::ios::binary);
        if (!random) {
            throw std::runtime_error("Failed to open /dev/random");
        }

        random.read(buffer_.data(), CHUNK_SIZE);
        
        if (!random) {
            throw std::runtime_error("Failed to read from /dev/random");
        }

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    void print_iteration_stats(size_t iteration, double duration_us) {
        std::cout << "Iteration " << iteration + 1 << "/" << NUM_EXPERIMENTS
                  << " completed in " << duration_us << " µs ("
                  << CHUNK_SIZE/(duration_us/1e6)/1e6 << " MB/s)\n";
    }

    void analyze_results() {
        const double avg = std::accumulate(timings_.begin(), timings_.end(), 0.0) / timings_.size();
        const auto [min, max] = std::minmax_element(timings_.begin(), timings_.end());

        std::cout << "\n=== Benchmark Results ===\n"
                  << "Samples: " << NUM_EXPERIMENTS << "\n"
                  << "Chunk size: " << CHUNK_SIZE/(1024*1024) << " MB\n"
                  << "Average time: " << avg << " µs\n"
                  << "Minimum time: " << *min << " µs\n"
                  << "Maximum time: " << *max << " µs\n"
                  << "Average throughput: " << CHUNK_SIZE/(avg/1e6)/1e6 << " MB/s\n";
    }

    void visualize_results() {
        GraphPlotter plotter;
        plotter.setTitle("Random Read Performance");
        plotter.setXLabel("Iteration");
        plotter.setYLabel("Time (µs)");
        plotter.addGraph("8MB Read Latency", timings_);
        plotter.plot();
    }
};

int main() {
    try {
        constexpr size_t NUM_EXPERIMENTS = 1000;
        constexpr size_t CHUNK_SIZE = 8 * 1024 * 1024; // 8 MB

        RandomBenchmark benchmark(NUM_EXPERIMENTS, CHUNK_SIZE);
        benchmark.run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
