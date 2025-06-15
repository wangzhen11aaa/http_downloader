#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include "glog/logging.h"
#include <cassert>
#include <condition_variable>
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
// Wait,Receive and execute task.
class ThreadPool {
public:
  ThreadPool() = default;
  ThreadPool(ThreadPool const &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool operator=(const ThreadPool &) = delete;
  ThreadPool operator=(ThreadPool &&) = delete;

  ThreadPool(int n);

  template <class Fn, typename... Args>
  std::future<std::invoke_result_t<Fn, Args...>> enqueue(Fn &&f,
                                                         Args &&...args);
  ~ThreadPool();

private:
  int nr_threads_;
  std::mutex mutex_;
  std::queue<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;
  std::condition_variable cv_;
  bool stop_;
};
#include "ThreadPool.tpp"
#endif