#include "Context.h"
#include "HTTPDownLoadHelper.h"
#include "MUtil.h"
#include "Task.h"
#include "glog/logging.h"
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <vector>
using namespace std;

// 初始化Context,并调用线程池下载
void downLoad(shared_ptr<Context> global_context_ptr, vector<string> &url_v,
              int part_size) {
  DownLoadTask t{};
  for (auto &url : url_v) {
    double fileSize = 0.0;
    bool supportRange = false;
    auto HTTPDownLoadHelperPtr = HTTPDownLoadHelper::getInstance();
    HTTPDownLoadHelperPtr->getHTTPRemoteFileSize(supportRange, url, fileSize);
    if (supportRange) {
      DLOG(INFO) << "Remote HTTP Server support Range" << boolalpha
                 << supportRange << endl;
    }
    string file_name;

    if (MUtil::parseFileName(url, file_name)) {
      if (!MUtil::generateDownloadDir(file_name)) {
        DLOG(INFO) << "Create directory failed" << endl;
        continue;
      }
    }
    global_context_ptr->add_file_size_map(file_name, fileSize);
    vector<future<double>> vf{};
    if (!supportRange) {
      global_context_ptr->add_file_future_map(
          file_name, -1,
          global_context_ptr->getDownloadThreadPool()->enqueue(
              &DownLoadTask::execute, &t, url, MUtil::getFilePath(file_name)));
    } else {
      double totalSz = 0.0;
      using ll = long long;
      ll s = 0, z = s + part_size;
      int part_num = 0;
      while (s < fileSize) {
        string rangeStr = MUtil::generateRange(s, min(z, (ll)fileSize));
        DLOG(INFO) << "rangeStr: " << rangeStr << endl;
        global_context_ptr->add_file_future_map(
            file_name, part_num,
            global_context_ptr->getDownloadThreadPool()->enqueue(
                &DownLoadTask::execute_for_part, &t, url,
                MUtil::getFilePath(file_name, part_num), rangeStr));
        global_context_ptr->file_part_range_map_[file_name].push_back(rangeStr);
        global_context_ptr->file_url_map_[file_name] = url;
        s = z + 1;
        z = s + part_size;
        part_num++;
      }
      global_context_ptr->add_file_parts_map(file_name, part_num);
    }
  }
}

void waitAndCombine(shared_ptr<Context> global_context_ptr, int part_size) {
  CombineTask combine_task{};
  DownLoadTask t{};
  int cnt = 0;
  do {
    for (auto &it : global_context_ptr->get_file_future_map()) {
      string file_name = it.first;
      if (!global_context_ptr->get_file_parts(file_name)) {
        // 如果当前下载没有part.
        cnt++;
        continue;
      }
      double fz = global_context_ptr->get_file_size(file_name);
      DLOG(INFO) << "File size: " << fz << endl;
      auto fv = std::move(it.second);
      // 用户存储没有传输正确的结果.
      vector<pair<int, future<double>>> nfv{};
      for (auto &&pair_it_ : fv) {
        int part_index = pair_it_.first;
        int part_num = global_context_ptr->get_file_parts(file_name);
        DLOG(INFO) << "part_num: " << part_num << endl;
        string url = global_context_ptr->file_url_map_[file_name];
        // 使用wait_for是为了获取future_status的状态,可以重试下载功能.
        // TODO 需要捕获Exception,以便终止任务.
        auto status = pair_it_.second.wait_for(global_context_ptr->wait_time_);
        // auto status = pair_it_.second.wait(global_context_ptr->wait_time_);
        if (status == std::future_status::ready) {
          double read_size = pair_it_.second.get();
          DLOG(INFO) << "read size: " << read_size << endl;
          if (part_index == -1) {
            // 如果数据下载不对，那么需要重传 TODO 可以设置重试下载次数
            if (read_size != global_context_ptr->get_file_size(file_name)) {
              nfv.push_back(
                  {part_index,
                   std::move(global_context_ptr->getDownloadThreadPool()
                                 ->enqueue(&DownLoadTask::execute, &t, url,
                                           MUtil::getFilePath(file_name)))});
            } else {
              // 已经读取的数量,直接读取
              global_context_ptr->file_has_read_size_map_[file_name] =
                  read_size;
            }
          } else {
            string rangeStr =
                global_context_ptr->file_part_range_map_[file_name][part_index];
            // 当前读取完毕
            if (read_size == part_size + 1) {
              global_context_ptr->file_has_read_size_map_[file_name] +=
                  read_size;
              DLOG(INFO)
                  << "Have read " << file_name << ","
                  << global_context_ptr->file_has_read_size_map_[file_name];
            } else if (read_size != part_size + 1 &&
                       part_index != part_num - 1) {
              DLOG(INFO) << "part_index" << part_index << "read error";
              nfv.push_back(
                  {part_index,
                   std::move(
                       global_context_ptr->getDownloadThreadPool()->enqueue(
                           &DownLoadTask::execute_for_part, &t, url,
                           MUtil::getFilePath(file_name, part_index),
                           rangeStr))});
            }
            // 当前range读取数量不对,或者是最后一个range,但总数不对.
            // 重传不移动future，我们认为此次读取失败.
            else {
              if (global_context_ptr->file_has_read_size_map_[file_name] +
                      read_size ==
                  fz) {
                global_context_ptr->file_has_read_size_map_[file_name] = fz;
                DLOG(INFO) << "END Reading from server " << file_name;
                break;
              } else {
                DLOG(INFO)
                    << "Have read"
                    << global_context_ptr->file_has_read_size_map_[file_name] +
                           read_size
                    << endl;
                DLOG(INFO) << "File size: " << fz << endl;
                DLOG(INFO) << "fileParts: "
                           << global_context_ptr->get_file_parts(file_name);
                DLOG(INFO) << "Refetch part_index: " << file_name << ","
                           << part_index << endl;

                nfv.push_back(
                    {part_index,
                     std::move(
                         global_context_ptr->getDownloadThreadPool()->enqueue(
                             &DownLoadTask::execute_for_part, &t, url,
                             MUtil::getFilePath(file_name, part_index),
                             rangeStr))});
              }
            }
          }
        } else if (status == std::future_status::timeout) {
          if (part_index == -1) {
            // 如果数据下载不对，那么需要重传
            global_context_ptr->add_file_future_map(
                file_name, -1,
                global_context_ptr->getDownloadThreadPool()->enqueue(
                    &DownLoadTask::execute, &t, url,
                    MUtil::getFilePath(file_name)));
          } else {
            string rangeStr =
                global_context_ptr->file_part_range_map_[file_name][part_index];
            nfv.push_back(
                {part_index,
                 std::move(global_context_ptr->getDownloadThreadPool()->enqueue(
                     &DownLoadTask::execute_for_part, &t, url,
                     MUtil::getFilePath(file_name, part_index), rangeStr))});
          }
        } else {
          // 如果future status 为 deferred.
          DLOG(INFO) << "nvf added" << endl;
          nfv.push_back({pair_it_.first, std::move(pair_it_.second)});
        }
        }

      // 如果读取数据一致
      if (global_context_ptr->file_has_read_size_map_[file_name] == fz) {
        cnt++;
        DLOG(INFO) << "Read done \n";
        DLOG(INFO) << global_context_ptr->file_has_read_size_map_[file_name]
                   << endl;
        DLOG(INFO) << "Read from server successully" << fz << endl;
        global_context_ptr->add_combile_file_future_map(
            file_name, global_context_ptr->getCombinerThreadPool()->enqueue(
                           &CombineTask::execute, &combine_task, file_name,
                           global_context_ptr->get_file_parts(file_name)));
        DLOG(INFO) << "Combine file size: " << fz << endl;
        break;
      } else {
        DLOG(INFO) << "nfv'size(): " << nfv.size() << endl;
        if (nfv.size() > 0) {
          global_context_ptr->add_file_future_vec(file_name, std::move(nfv));
        }
      }
    }
  } while (cnt != global_context_ptr->get_file_count());
  for (auto &[file_name, sz] : global_context_ptr->get_file_size_map()) {
    if (!global_context_ptr->get_file_parts(file_name)) {
      continue;
    }
    DLOG(INFO) << "file_name: " << file_name << endl;
    auto fu = std::move(global_context_ptr->get_combile_file_future(file_name));
    DLOG(INFO) << "get combine future";
    fu.wait();
    auto k = fu.get();
    if (k == sz) {
      DLOG(INFO) << "Combine file " << file_name << "successfully\n";
    }
  }
}

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = 1;
  //FLAGS_log_dir = "./log";
  google::InitGoogleLogging(argv[0]);
  DLOG(INFO) << "Running" << endl;

  vector<string> urls{};
  int concurrency = 0;
  int part_size = 0;
  MUtil::initOps(urls, concurrency, part_size, argc, argv);
  std::shared_ptr<Context> global_context_ptr =
      make_shared<Context>(concurrency);
  downLoad(global_context_ptr->getContext(), urls, part_size);
  waitAndCombine(global_context_ptr->getContext(), part_size);  
  return 0;
}