#include <iostream>
#include <boost/thread.hpp>
#include <queue>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <cmath>

enum class TaskType {
    FACTORIAL = 1,
    FIBONACCI,
    SUM_DIGITS,
    IS_PRIME,
    GCD,
    REVERSE_NUMBER
};

struct Task {
    int id;
    TaskType type;
    long long arg1;
    long long arg2;
    std::string result;
};

std::queue<Task> taskQueue;

std::mutex queueMutex;
std::condition_variable queueCondition;

std::mutex resultMutex;
std::mutex coutMutex;

std::vector<Task> results;

std::atomic<int> completedTasks(0);
bool tasksFinished = false;

unsigned long long factorial(int n) {
    unsigned long long result = 1;

    for (int i = 2; i <= n; ++i) {
        result *= i;
    }

    return result;
}

unsigned long long fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;

    unsigned long long a = 0;
    unsigned long long b = 1;

    for (int i = 2; i <= n; ++i) {
        unsigned long long temp = a + b;
        a = b;
        b = temp;
    }

    return b;
}

int sumDigits(long long n) {
    n = std::abs(n);

    int sum = 0;

    while (n > 0) {
        sum += n % 10;
        n /= 10;
    }

    return sum;
}

bool isPrime(long long n) {
    if (n <= 1) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (long long i = 5; i <= n / i; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }

    return true;
}

long long gcd(long long a, long long b) {
    a = std::abs(a);
    b = std::abs(b);

    while (b != 0) {
        long long temp = b;
        b = a % b;
        a = temp;
    }

    return a;
}

long long reverseNumber(long long n) {
    bool negative = n < 0;
    n = std::abs(n);

    long long reversed = 0;

    while (n > 0) {
        reversed = reversed * 10 + n % 10;
        n /= 10;
    }

    return negative ? -reversed : reversed;
}

std::string getTaskName(TaskType type) {
    switch (type) {
    case TaskType::FACTORIAL:
        return "Факториал";
    case TaskType::FIBONACCI:
        return "Фибоначчи";
    case TaskType::SUM_DIGITS:
        return "Сумма цифр";
    case TaskType::IS_PRIME:
        return "Проверка на простоту";
    case TaskType::GCD:
        return "НОД";
    case TaskType::REVERSE_NUMBER:
        return "Реверс числа";
    default:
        return "Неизвестная задача";
    }
}

void processTask(Task& task) {
    switch (task.type) {
    case TaskType::FACTORIAL:
        task.result = std::to_string(factorial(static_cast<int>(task.arg1)));
        break;

    case TaskType::FIBONACCI:
        task.result = std::to_string(fibonacci(static_cast<int>(task.arg1)));
        break;

    case TaskType::SUM_DIGITS:
        task.result = std::to_string(sumDigits(task.arg1));
        break;

    case TaskType::IS_PRIME:
        task.result = isPrime(task.arg1) ? "Да" : "Нет";
        break;

    case TaskType::GCD:
        task.result = std::to_string(gcd(task.arg1, task.arg2));
        break;

    case TaskType::REVERSE_NUMBER:
        task.result = std::to_string(reverseNumber(task.arg1));
        break;
    }
}

void worker(int workerId) {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            queueCondition.wait(lock, [] {
                return !taskQueue.empty() || tasksFinished;
            });

            if (taskQueue.empty() && tasksFinished) {
                break;
            }

            task = taskQueue.front();
            taskQueue.pop();
        }

        processTask(task);

        {
            std::lock_guard<std::mutex> lock(resultMutex);
            results.push_back(task);
        }

        ++completedTasks;

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Поток " << workerId
                      << " выполнил задачу #" << task.id << std::endl;
        }
    }
}

void addTask(int id, TaskType type, long long arg1, long long arg2 = 0) {
    Task task;
    task.id = id;
    task.type = type;
    task.arg1 = arg1;
    task.arg2 = arg2;
    task.result = "";

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(task);
    }

    queueCondition.notify_one();
}

int main() {
    const int THREAD_COUNT = 3;

    auto startTime = std::chrono::steady_clock::now();

    boost::thread_group workers;

    for (int i = 1; i <= THREAD_COUNT; ++i) {
        workers.create_thread(boost::bind(worker, i));
    }

    addTask(1, TaskType::FACTORIAL, 5);
    addTask(2, TaskType::FIBONACCI, 10);
    addTask(3, TaskType::SUM_DIGITS, 12345);
    addTask(4, TaskType::IS_PRIME, 97);
    addTask(5, TaskType::GCD, 48, 18);
    addTask(6, TaskType::REVERSE_NUMBER, 123456);

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        tasksFinished = true;
    }

    queueCondition.notify_all();

    workers.join_all();

    auto endTime = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    ).count();

    std::sort(results.begin(), results.end(), [](const Task& a, const Task& b) {
        return a.id < b.id;
    });

    std::cout << "\n===== РЕЗУЛЬТАТЫ =====\n";

    for (const Task& task : results) {
        std::cout << "Задача #" << task.id << ": "
                  << getTaskName(task.type) << " ";

        if (task.type == TaskType::GCD) {
            std::cout << "(" << task.arg1 << ", " << task.arg2 << ")";
        } else {
            std::cout << "(" << task.arg1 << ")";
        }

        std::cout << " = " << task.result << std::endl;
    }

    std::cout << "\nВыполнено задач: " << completedTasks.load() << std::endl;
    std::cout << "Общее время выполнения: " << duration << " мс" << std::endl;

    return 0;
}