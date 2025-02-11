#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <mutex>
#include <chrono>

std::mutex output_mutex; // To synchronize access to the console output

// Function to process a chunk of lines from the CSV file
void process_chunk(const std::vector<std::string>& chunk, int chunk_index, std::vector<std::string>& result, std::mutex& result_mutex) {
    for (const auto& line : chunk) {
        std::cout << "Processing: " << line << std::endl; // Debugging print
        std::lock_guard<std::mutex> lock(result_mutex);
        result.push_back("Processed: " + line);
    }
}

// Function to read the CSV file in parallel
void analyze_csv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    // Read the entire file into a vector of strings (each representing a line)
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    // Ensure at least 1 thread
    const size_t num_threads = std::max(std::thread::hardware_concurrency(), 1u);
    const size_t chunk_size = lines.size() / num_threads;

    std::vector<std::future<void>> futures;
    std::vector<std::vector<std::string>> results(num_threads);
    std::vector<std::mutex> result_mutexes(num_threads);

    // Launch threads to process each chunk of lines
    for (size_t i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? lines.size() : (i + 1) * chunk_size;
        std::vector<std::string> chunk(lines.begin() + start, lines.begin() + end);

        futures.push_back(std::async(std::launch::async, process_chunk, std::ref(chunk), i, std::ref(results[i]), std::ref(result_mutexes[i])));
    }

    // Wait for all threads to finish
    for (auto& fut : futures) {
        fut.get();
    }

    // Combine the results from all threads
    std::vector<std::string> final_result;
    for (const auto& result_chunk : results) {
        final_result.insert(final_result.end(), result_chunk.begin(), result_chunk.end());
    }

    // Output the processed results (for demonstration purposes)
    std::lock_guard<std::mutex> lock(output_mutex);
    std::cout << "Processed CSV data (first 10 rows):\n";
    for (size_t i = 0; i < std::min(size_t(10), final_result.size()); ++i) {
        std::cout << final_result[i] << std::endl;
    }
}

int main() {
    std::string filename = "data.csv";

    auto start_time = std::chrono::high_resolution_clock::now();
    analyze_csv(filename);
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "\nAnalysis complete. Total time: " << duration.count() << " seconds.\n";

    return 0;
}      