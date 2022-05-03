#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
using std::thread;
using std::max;
using std::min;

class ThreadPool
{
    using Task = std::function<void()>;
    // 执行任务的线程
    std::vector<std::thread> pool;
    // 任务队列
    std::queue<Task> tasks;
    // 保护任务队列的锁
    std::mutex taskQueueLock;
    // 条件变量，所有线程在该变量上等待。有任务加入任务队列时，唤醒该条件变量
    std::condition_variable cvTask;
    // 是否已经关闭，关闭后，无法再提交任务
    std::atomic<bool> isStopped;
    // 空闲线程个数
    std::atomic<uint32_t> idleThreadNumber;
public:
    inline ThreadPool(uint32_t size = 4) : isStopped{ false }
    {
        idleThreadNumber = max(1U, size);
        idleThreadNumber = min(idleThreadNumber.load(), MAX_THREAD_NUMBER);
        for (size = 0; size < idleThreadNumber; ++size)
        {   // 初始化任务执行线程
            pool.emplace_back(
                    [this]
                    {
                        while (!this->isStopped)
                        {
                            std::function<void()> task;
                            {   // 在条件变量上等待
                                std::unique_lock<std::mutex> lock{ this->taskQueueLock };
                                // 等待，第二个参数Predicate，用于解决假唤醒，see also: https://en.cppreference.com/w/cpp/thread/condition_variable/wait
                                this->cvTask.wait(lock,
                                                   [this] {
                                                       return this->isStopped.load() || !this->tasks.empty();
                                                   }
                                ); // wait 直到有 task
                                // 线程池已关闭且没有任务可以做
                                if (this->isStopped && this->tasks.empty())
                                    return;
                                // 使用了notify_all后，多个线程被唤醒，但可能该线程没有拿到任务，那就继续等待
                                if (this->tasks.empty()) continue;
                                task = std::move(this->tasks.front()); // 取一個 task
                                this->tasks.pop();
                            }
                            idleThreadNumber--;
                            task();
                            idleThreadNumber++;
                        }
                    }
            );
        }
    }
    inline ~ThreadPool()
    {
        isStopped.store(true);
        cvTask.notify_all(); // 唤醒所有线程，执行任务
        for (std::thread& thread : pool) {
            if (thread.joinable())
                thread.join(); // 等待线程结束
        }
    }

public:
    const uint32_t MAX_THREAD_NUMBER = thread::hardware_concurrency();
    // 提交任务接口
    template<class F, class... Args>
    auto commit(F&& f, Args&&... args) ->std::future<decltype(f(args...))>
    {
        if (isStopped.load())    // stop == true ??
            throw std::runtime_error("commit on ThreadPool is stopped.");

        // 获取函数的返回值类型
        using RetType = decltype(f(args...));
        // 创建一个新的packaged_task，可以通过packaged_task获得future，进而在函数执行完成后获取返回值
        auto task = std::make_shared<std::packaged_task<RetType()> >(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<RetType> future = task->get_future();
        {    // 将任务添加到任务队列
            std::lock_guard<std::mutex> lock{ taskQueueLock };
            //
            tasks.emplace(
                    [task]()
                    {
                        (*task)();
                    }
            );
        }
        // 唤醒一个线程进行执行
        cvTask.notify_one();
        return future;
    }

    //空闲线程数量
    [[nodiscard]] uint32_t GetIdleThreadNumber() const { return idleThreadNumber; }
    // 是否当前所有任务已完成
    bool IsAllFinished(){
        std::lock_guard<std::mutex> lock{ taskQueueLock };
        return tasks.empty();
    }
};


