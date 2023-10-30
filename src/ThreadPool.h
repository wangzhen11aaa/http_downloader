#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#endif
#include <cassert>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdio.h>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include "glog/logging.h"
using namespace std;
// Wait,Receive and execute task.
class ThreadPool {
public:
  ThreadPool() = default;
  ThreadPool(ThreadPool const &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool operator=(const ThreadPool &) = delete;
  ThreadPool operator=(ThreadPool &&) = delete;
  ThreadPool(int n) : nr_threads(n), stop(false) {
    for (int i = 0; i < nr_threads; i++) {
      thread worker([this]() {
        while (true) {
          function<void()> task;
          {
            unique_lock<mutex> lk(mutex_);
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
      workers.emplace_back(std::move(worker));
    }
  }

 // enqueue方法返回一个future。这里并不阻塞，而是将task传送给tasks队列，这些队列被监听队列的线程执行。
 // 线程在另外一个stack执行，enqueue方法返回一个future。future是一个存储线程运行结果的结构体,通过调用future的wait_for方法对调用结果进行同步()可能涉及到线程间栈的通信。即执行任务的线程(另外一个栈)将结果放到main thread的 栈(future所在的栈)。在main栈上的位置通过std::move发生了变化。跨栈通信。
  template <class Fn, typename... Args>
  future<invoke_result_t<Fn, Args...>> enqueue(Fn &&f, Args &&...args) {
    using return_type = std::invoke_result_t<Fn, Args...>;
    auto task = make_shared<packaged_task<return_type()>>(
        bind(std::forward<Fn>(f), std::forward<Args>(args)...));
    auto res = task->get_future();
    {
      lock_guard<std::mutex> lk(mutex_);
      tasks.emplace([task]() -> void { (*task)(); });
    }
    cv.notify_one();
    return std::move(std::move(res));
  }

  ~ThreadPool() {
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

private:
  int nr_threads;
  mutex mutex_;
  queue<function<void()>> tasks;
  vector<thread> workers;
  condition_variable cv;
  bool stop;
};