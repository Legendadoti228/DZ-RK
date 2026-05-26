#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <random>
#include <chrono>
#include <vector>

const int ZONES = 2;
const int CHECKS = 10;
const int THRESHOLD = 3;
const int REFILL_AMOUNT = 5;

struct Zone {
    int stock = 0;
    bool need_refill = false;
    std::mutex mtx;
    std::condition_variable cv;
};

std::vector<Zone> zones(ZONES);

std::mutex common_mtx;
std::condition_variable refill_cv;

std::atomic<bool> stop(false);
std::atomic<int> checks_done(0);
std::atomic<int> refills_done(0);

bool any_need_refill() {
    for (int i = 0; i < ZONES; ++i) {
        std::lock_guard<std::mutex> lock(zones[i].mtx);
        if (zones[i].need_refill) {
            return true;
        }
    }
    return false;
}

void checker() {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> zone_dist(0, ZONES - 1);
    std::uniform_int_distribution<> sleep_dist(200, 700);

    for (int i = 1; i <= CHECKS; ++i) {
        int zone_id = zone_dist(gen);

        std::unique_lock<std::mutex> lock(zones[zone_id].mtx);

        std::cout << "[Checker] Check " << i
                  << ". Zone " << zone_id
                  << " stock = " << zones[zone_id].stock << std::endl;

        if (zones[zone_id].stock < THRESHOLD) {
            std::cout << "[Checker] Zone " << zone_id
                      << " needs refill" << std::endl;

            zones[zone_id].need_refill = true;

            refill_cv.notify_one();

            zones[zone_id].cv.wait(lock, [&] {
                return !zones[zone_id].need_refill;
            });

            std::cout << "[Checker] Zone " << zone_id
                      << " refill completed. New stock = "
                      << zones[zone_id].stock << std::endl;
        }

        checks_done++;

        lock.unlock();

        std::this_thread::sleep_for(
            std::chrono::milliseconds(sleep_dist(gen))
        );
    }

    stop = true;
    refill_cv.notify_one();
}

void refiller() {
    while (!stop.load()) {
        std::unique_lock<std::mutex> common_lock(common_mtx);

        refill_cv.wait(common_lock, [] {
            return any_need_refill() || stop.load();
        });

        if (stop.load()) {
            break;
        }

        common_lock.unlock();

        for (int i = 0; i < ZONES; ++i) {
            std::lock_guard<std::mutex> zone_lock(zones[i].mtx);

            if (zones[i].need_refill) {
                zones[i].stock += REFILL_AMOUNT;
                zones[i].need_refill = false;

                refills_done++;

                std::cout << "[Refiller] Zone " << i
                          << " refilled by " << REFILL_AMOUNT
                          << ". New stock = " << zones[i].stock
                          << std::endl;

                zones[i].cv.notify_one();
            }
        }
    }
}

int main() {
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < ZONES; ++i) {
        zones[i].stock = 2;
        zones[i].need_refill = false;
    }

    std::thread t1(checker);
    std::thread t2(refiller);

    t1.join();
    t2.join();

    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();

    std::cout << "\n=== Final stock ===\n";

    for (int i = 0; i < ZONES; ++i) {
        std::cout << "Zone " << i
                  << ": " << zones[i].stock << std::endl;
    }

    std::cout << "\nChecks done: " << checks_done.load() << std::endl;
    std::cout << "Refills done: " << refills_done.load() << std::endl;
    std::cout << "Total time: " << duration << " ms" << std::endl;

    return 0;
}