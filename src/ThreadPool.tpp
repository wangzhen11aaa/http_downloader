#include "ThreadPool.h"

ThreadPool::ThreadPool(int n) : nr_threads(n), stop(false) {
  for (int i = 0; i < nr_threads; i++) {
    std::thread worker([this]() {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lk(mutex_);
          cv.wait(lk, [this]() { return stop || tasks.size() > 0; });
          if (stop || tasks.empty()) {
            DLOG(INFO) << "Signal stop received\n";
            return;
          }
          task = std::move(tasks.front());
          tasks.pop();
        }
        task();
      }
    });
    // workers.emplace_back(std::move(worker));
    workers.push_back(std::move(worker));
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
    tasks.push([task]() -> void { (*task)(); });
  }
  cv.notify_one();
  return std::move(res);
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lk(mutex_);
    stop = true;
  }
  cv.notify_all();
  DLOG(INFO) << "ThreadPool destrcuted \n";
  for (auto &worker : workers) {
    worker.join();
  }
  DLOG(INFO) << "Resource freed.\n";
}