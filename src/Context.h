#ifndef CONTEXT_H
#define CONTEXT_H
#endif
#include "ThreadPool.h"
#include "glog/logging.h"
#include <chrono>
#include <future>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
class Context : public enable_shared_from_this<Context> {
public:
  Context(int downloader_cnt) : downloaders_cnt_(downloader_cnt) {
    downloader_threadpool_ptr_ = make_shared<ThreadPool>(downloader_cnt);
    combiner_threadpool_ptr_ = make_shared<ThreadPool>(1);
  }

  std::shared_ptr<Context> getContext() { return shared_from_this(); }
  // 获取下载线程池
  shared_ptr<ThreadPool> getDownloadThreadPool() {
    return downloader_threadpool_ptr_;
  }
  // 获取合并线程池
  shared_ptr<ThreadPool> getCombinerThreadPool() {
    return combiner_threadpool_ptr_;
  }

  void add_file_size_map(string &file_name, double filesize) {
    if (fz_map_.count(file_name)) {
      DLOG(INFO) << "File already exists";
    } else {
      fz_map_.insert(make_pair(file_name, filesize));
    }
  }

  double get_file_size(const string &file_name) { return fz_map_[file_name]; }

  unordered_map<string, double> &get_file_size_map() { return fz_map_; }

  void add_file_future_map(const string &file_name, int idx,
                           future<double> fut) {
    file_future_map_[file_name].push_back({idx, std::move(fut)});
  }
  void add_file_future_vec(const string &file_name,
                           vector<pair<int,future<double>>> &&other) {
    auto t = std::move(file_future_map_[file_name]);
    file_future_map_[file_name] = std::move(other);
  }

  void add_file_dir_map(const string &file_name, const string &dirname) {
    if (file_dir_map_.count(file_name)) {
      return;
    }
    file_dir_map_[file_name] = dirname;
  }

  unordered_map<string, vector<pair<int,future<double>>>> &get_file_future_map() {
    return file_future_map_;
  }

  vector<pair<int, future<double>>> &get_file_future(const string &file_name) {
    return file_future_map_[file_name];
  }
  // 记录一个file有多少个parts.
  void add_file_parts_map(string &file_name, int parts) {
    assert(file_dir_map_.count(file_name) == 0);
    file_parts_map_[file_name] = parts;
  }

  int get_file_parts(const string &file_name) {
    return file_parts_map_[file_name];
  }
  // string fetch_dirname(string &file_name) { return file_dir_map_[file_name];
  // }

  void add_combile_file_future_map(const string &file_name,
                                   future<double> &&fut) {
    DLOG(INFO) << "combine future has added \n";
    combile_file_future_map_[file_name] = std::move(fut);
  }

  future<double> &&get_combile_file_future(const string &file_name) {
    return std::move(combile_file_future_map_[file_name]);
  }

  int get_file_count() const { return fz_map_.size(); }
  ~Context() { DLOG(INFO) << "Context freed \n"; }
  // TOOD 可以根据当前任务的下载速度来计算超时时间并重新赋值给wait_time.
  std::chrono::seconds wait_time_ = 10s;
  unordered_map<string, double> file_has_read_size_map_{};
  unordered_map<string, vector<string>> file_part_range_map_{};
  unordered_map<string, string> file_url_map_{};
private:
  Context() = default;
  Context(const Context &) = default;
  Context &operator=(const Context &) = default;
  int downloaders_cnt_;
  vector<string> urls_;

  // 为每个下载文件单独创建一个下载目录
  // unordered_map<string, string> url_download_dir_map_;
  // 存储需要下载的文件名
  unordered_map<string, double> fz_map_{};
  unordered_map<string, int> file_parts_map_{};
  // 将文件名与future做映射,使用future.wait_for(w_t)来循环遍历任务状态.
  unordered_map<string, vector<pair<int, future<double>>>> file_future_map_{};
  // 将文件和目录进行映射 TODO.
  unordered_map<string, string> file_dir_map_{};
  // 每个url download与子任务个数的映射.
  unordered_map<string, int> file_taks_cnt_map_{};
  // 在main_thread中奖url任务分解成小的task后，就将这些task交给downloader.
  shared_ptr<ThreadPool> downloader_threadpool_ptr_{};

  // 当某个下载任务得到了所有有效的future,将合并任务交给combiner.
  shared_ptr<ThreadPool> combiner_threadpool_ptr_;

  unordered_map<string, future<double>> combile_file_future_map_;
};
