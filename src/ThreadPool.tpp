#include "ThreadPool.h"

ThreadPool::ThreadPool(int n) : nr_threads_(n), stop_(false) {
  for (int i = 0; i < nr_threads_; i++) {
    std::thread worker([this]() {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lk(mutex_);
          cv_.wait(lk, [this]() { return stop_ || tasks_.size() > 0; });
          if (stop_ || tasks_.empty()) {
            DLOG(INFO) << "Signal stop_ received\n";
            return;
          }
          task = std::move(tasks_.front());
          tasks_.pop();
        }
        task();
      }
    });
    // workers_.emplace_back(std::move(worker));
    workers_.push_back(std::move(worker));
  }
}

// Asychronously execute task.
template <class Fn, typename... Args>
std::future<std::invoke_result_t<Fn, Args...>>
ThreadPool::enqueue(Fn &&f, Args &&...args) {
  using return_type = std::invoke_result_t<Fn, Args...>;
  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<Fn>(f), std::forward<Args>(args)...));
  // auto res = std::async(task);
  auto res = task->get_future();
  {
    std::lock_guard<std::mutex> lk(mutex_);
    tasks_.push([task]() -> void { (*task)(); });
  }
  cv_.notify_one();
  return std::move(res);
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lk(mutex_);
    stop_ = true;
  }
  cv_.notify_all();
  DLOG(INFO) << "ThreadPool destrcuted \n";
  for (auto &worker : workers_) {
    worker.join();
  }
  DLOG(INFO) << "Resource freed.\n";
}