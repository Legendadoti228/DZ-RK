#include <iostream>
#include <thread>
#include <semaphore>
#include <chrono>
#include <random>
#include <atomic>

const int ITEMS = 5;

std::counting_semaphore<ITEMS> sem_processing(0);
std::counting_semaphore<ITEMS> sem_packaging(0);

std::atomic<int> loaded_count(0);
std::atomic<int> processed_count(0);
std::atomic<int> packaged_count(0);

void loading() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> time_dist(1, 2);

    for (int i = 1; i <= ITEMS; ++i) {
        int t = time_dist(gen);

        std::cout << "[Loading] Item " << i
                  << " started (" << t << " sec)" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(t));

        std::cout << "[Loading] Item " << i
                  << " completed" << std::endl;

        loaded_count++;

        sem_processing.release();
    }
}

void processing() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> time_dist(1, 2);

    for (int i = 1; i <= ITEMS; ++i) {
        sem_processing.acquire();

        int t = time_dist(gen);

        std::cout << "[Processing] Item " << i
                  << " started (" << t << " sec)" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(t));

        std::cout << "[Processing] Item " << i
                  << " completed" << std::endl;

        processed_count++;

        sem_packaging.release();
    }
}

void packaging() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> time_dist(1, 2);

    for (int i = 1; i <= ITEMS; ++i) {
        sem_packaging.acquire();

        int t = time_dist(gen);

        std::cout << "[Packaging] Item " << i
                  << " started (" << t << " sec)" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(t));

        std::cout << "[Packaging] Item " << i
                  << " completed" << std::endl;

        packaged_count++;
    }
}

int main() {
    auto start = std::chrono::steady_clock::now();

    std::thread t1(loading);
    std::thread t2(processing);
    std::thread t3(packaging);

    t1.join();
    t2.join();
    t3.join();

    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();

    std::cout << "\n=== Result ===\n";
    std::cout << "Loaded items: " << loaded_count.load() << std::endl;
    std::cout << "Processed items: " << processed_count.load() << std::endl;
    std::cout << "Packaged items: " << packaged_count.load() << std::endl;
    std::cout << "Total time: " << duration << " ms" << std::endl;

    return 0;
}