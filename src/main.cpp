#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include <future>
#include <atomic>

constexpr long long size = 100000000;

// four threads to execute
constexpr long long first = 25000000;
constexpr long long second = 50000000;
constexpr long long third = 75000000;
constexpr long long fourth = 100000000;
std::mutex sum_mutex;

class spinlock
{
private:
    std::atomic_flag flag;

public:
    spinlock() : flag(ATOMIC_FLAG_INIT) {}
    void lock()
    {
        while (flag.test_and_set(std::memory_order_acquire))
            ;
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};

spinlock sp_lock;

void my_sum(unsigned long long &sum,
            const std::vector<int> &vec,
            unsigned long long beg,
            unsigned long long end)
{
    unsigned long long tmp_sum = 0;
    for (auto i = beg; i < end; ++i)
    {
        tmp_sum += vec[i];
    }
    std::lock_guard<std::mutex> lock(sum_mutex);
    sum += tmp_sum;
}

unsigned long long my_sum_async(const std::vector<int> &vec,
                                unsigned long long beg,
                                unsigned long long end)
{
    unsigned long long tmp_sum = 0;
    for (auto i = beg; i < end; ++i)
    {
        tmp_sum += vec[i];
    }
    return tmp_sum;
}

void my_sum_promise(std::promise<unsigned long long> &&prom,
                    const std::vector<int> &vec,
                    unsigned long long beg,
                    unsigned long long end)
{
    unsigned long long tmp_sum = 0;
    for (auto i = beg; i < end; ++i)
    {
        tmp_sum += vec[i];
    }
    prom.set_value(tmp_sum);
}

void my_sum_spinlock(unsigned long long &sum,
                     const std::vector<int> &vec,
                     unsigned long long beg,
                     unsigned long long end)
{
    unsigned long long tmp_sum = 0;
    for (auto i = beg; i < end; ++i)
    {
        tmp_sum += vec[i];
    }
    sp_lock.lock();
    sum += tmp_sum;
    sp_lock.unlock();
}

int main()
{
    std::cout
        << "hardware concurrency: "
        << std::thread::hardware_concurrency()
        << std::endl;

    std::vector<int> random_values;
    random_values.reserve(size);

    std::random_device seed;
    std::mt19937 engine(seed());
    std::uniform_int_distribution<> uniformDist(1, 10);

    for (long long i = 0; i < size; ++i)
    {
        random_values.push_back(uniformDist(engine));
    }

    // 1 single thread - loop sum
    const auto sta = std::chrono::steady_clock::now();
    unsigned long long sum = 0;
    for (auto n : random_values)
        sum += n;

    const std::chrono::duration<double> dur =
        std::chrono::steady_clock::now() - sta;

    std::cout
        << "Time for single thread - loop sum: "
        << dur.count()
        << " seconds"
        << std::endl;

    std::cout << "Result: " << sum << std::endl;

    // 2.1 multi-threads - raw threads with mutex
    const auto sta_1 = std::chrono::steady_clock::now();
    unsigned long long sum_1 = 0;

    std::thread t1(my_sum, std::ref(sum_1), std::cref(random_values), 0, first);
    std::thread t2(my_sum, std::ref(sum_1), std::cref(random_values), first, second);
    std::thread t3(my_sum, std::ref(sum_1), std::cref(random_values), second, third);
    std::thread t4(my_sum, std::ref(sum_1), std::cref(random_values), third, fourth);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    const std::chrono::duration<double> dur_1 =
        std::chrono::steady_clock::now() - sta_1;

    std::cout
        << "Time for multi-threads - raw threads with mutex: "
        << dur_1.count()
        << " seconds"
        << std::endl;

    std::cout << "Result: " << sum_1 << std::endl;

    // 2.2 multi-threads - async
    const auto sta_2 = std::chrono::steady_clock::now();
    unsigned long long sum_2 = 0;

    std::future<unsigned long long> fut = std::async(std::launch::async, my_sum_async, std::cref(random_values), 0, first);
    std::future<unsigned long long> fut_1 = std::async(std::launch::async, my_sum_async, std::cref(random_values), first, second);
    std::future<unsigned long long> fut_2 = std::async(std::launch::async, my_sum_async, std::cref(random_values), second, third);
    std::future<unsigned long long> fut_3 = std::async(std::launch::async, my_sum_async, std::cref(random_values), third, fourth);

    sum_2 = fut.get() + fut_1.get() + fut_2.get() + fut_3.get();

    const std::chrono::duration<double> dur_2 =
        std::chrono::steady_clock::now() - sta_2;

    std::cout
        << "Time for multi-threads - async: "
        << dur_2.count()
        << " seconds"
        << std::endl;
    std::cout << "Result: " << sum_2 << std::endl;

    // 2.3 multi-threads - promise and future
    const auto sta_3 = std::chrono::steady_clock::now();
    unsigned long long sum_3 = 0;

    std::promise<unsigned long long> prom;
    std::promise<unsigned long long> prom_1;
    std::promise<unsigned long long> prom_2;
    std::promise<unsigned long long> prom_3;

    std::future<unsigned long long> fut_p = prom.get_future();
    std::future<unsigned long long> fut_p_1 = prom_1.get_future();
    std::future<unsigned long long> fut_p_2 = prom_2.get_future();
    std::future<unsigned long long> fut_p_3 = prom_3.get_future();

    std::thread t_p_1(my_sum_promise, std::move(prom), std::cref(random_values), 0, first);
    std::thread t_p_2(my_sum_promise, std::move(prom_1), std::cref(random_values), first, second);
    std::thread t_p_3(my_sum_promise, std::move(prom_2), std::cref(random_values), second, third);
    std::thread t_p_4(my_sum_promise, std::move(prom_3), std::cref(random_values), third, fourth);

    sum_3 = fut_p.get() + fut_p_1.get() + fut_p_2.get() + fut_p_3.get();

    t_p_1.join();
    t_p_2.join();
    t_p_3.join();
    t_p_4.join();

    const std::chrono::duration<double> dur_3 =
        std::chrono::steady_clock::now() - sta_3;

    std::cout
        << "Time for multi-threads - promise and future: "
        << dur_3.count()
        << " seconds"
        << std::endl;
    std::cout << "Result: " << sum_3 << std::endl;

    // 2.4 multi-threads - raw threads with spinlock
    const auto sta_4 = std::chrono::steady_clock::now();
    unsigned long long sum_4 = 0;

    std::thread t_sp_1(my_sum_spinlock, std::ref(sum_4), std::cref(random_values), 0, first);
    std::thread t_sp_2(my_sum_spinlock, std::ref(sum_4), std::cref(random_values), first, second);
    std::thread t_sp_3(my_sum_spinlock, std::ref(sum_4), std::cref(random_values), second, third);
    std::thread t_sp_4(my_sum_spinlock, std::ref(sum_4), std::cref(random_values), third, fourth);

    t_sp_1.join();
    t_sp_2.join();
    t_sp_3.join();
    t_sp_4.join();

    const std::chrono::duration<double> dur_4 =
        std::chrono::steady_clock::now() - sta_4;

    std::cout
        << "Time for multi-threads - raw threads with spinlock: "
        << dur_4.count()
        << " seconds"
        << std::endl;

    std::cout << "Result: " << sum_4 << std::endl;

    return 0;
}
