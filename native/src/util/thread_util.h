#pragma once

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

namespace threading {

class ThreadBase {
public:
    ThreadBase()
        : is_running(false)
        , thread(nullptr) {}

    virtual ~ThreadBase() { join(); }

    void start() {
        if (is_running.load()) {
            return;
        }
        is_running.store(true);
        thread = std::make_unique<std::thread>([this]() { run(); });
    }

    void join() {
        if (!is_running.load()) {
            return;
        }
        is_running.store(false);
        thread->join();
        thread = nullptr;
    }

    bool isRunning() const { return is_running.load(); }

protected:
    virtual void run() = 0;

private:
    std::unique_ptr<std::thread> thread;
    std::atomic_bool is_running;
};

class Timer {
public:
    Timer(
        const std::chrono::milliseconds &duration,
        std::function<void()> on_expired,
        std::function<void()> on_canceled = nullptr)
        : thread(nullptr)
        , duration(duration)
        , on_expired(std::move(on_expired))
        , on_canceled(std::move(on_canceled)) {
        start();
    }

    ~Timer() { cancel(); }

    void cancel() {
        std::cout << __FUNCTION__ << std::endl;
        {
            std::lock_guard<std::mutex> lock(condition_mutex);
            cancelRequested = true;
            condition.notify_all();
        }

        {
            std::lock_guard<std::recursive_mutex> lock(thread_object_mutex);
            if (thread != nullptr) {
                thread->join();
                thread = nullptr;
            }
        }
    }

    [[nodiscard]] std::optional<bool> hasExpired() {
        std::unique_lock<std::mutex> lock(condition_mutex, std::defer_lock);
        if (!lock.try_lock()) {
            return std::nullopt;
        } else {
            return expired;
        }
    }

private:
    void start() {
        std::lock_guard<std::recursive_mutex> lock(thread_object_mutex);
        if (thread != nullptr) {
            cancel();
        }
        assert_(thread == nullptr);

        cancelRequested = false;
        expired = std::nullopt;
        const auto timeout = std::chrono::steady_clock::now() + duration;
        thread = std::make_unique<std::thread>([=]() { run(timeout); });
    }

    void run(std::chrono::steady_clock::time_point timeout) {
        std::cout << __FUNCTION__ << " started" << std::endl;

        std::unique_lock<std::mutex> cancel_lock(condition_mutex);
        if (condition.wait_until(cancel_lock, timeout, [&]() { return cancelRequested; })) {
            std::cout << "on_canceled: " << cancelRequested << std::endl;
            expired = false;
            if (on_canceled != nullptr) {
                on_canceled();
            }
        } else {
            std::cout << "on_expired: " << cancelRequested << std::endl;
            expired = true;
            on_expired();
        }

        std::cout << __FUNCTION__ << " finished" << std::endl;
    }

private:
    std::unique_ptr<std::thread> thread;
    std::recursive_mutex thread_object_mutex;

    std::condition_variable condition;
    std::mutex condition_mutex;

    bool cancelRequested = false;
    std::optional<bool> expired;

    const std::chrono::milliseconds duration;
    const std::function<void()> on_expired;
    const std::function<void()> on_canceled;
};

}  // namespace threading
