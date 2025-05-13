#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstdio>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "Context.h"
#include "HTTPDownLoadHelper.h"
#include "MUtil.h"
#include "Task.h"
#include "glog/logging.h"

// 初始化Context,并调用线程池下载
void downLoad(std::shared_ptr<Context> global_context_ptr,
              std::vector<std::string> &url_v, int part_size) {
  for (auto &url : url_v) {
    double fileSize = 0.0;
    bool supportRange = false;
    auto HTTPDownLoadHelperPtr =
        http_download::HTTPDownLoadHelper::getInstance();
    HTTPDownLoadHelperPtr->getHTTPRemoteFileSize(supportRange, url, fileSize);
    if (supportRange) {
      DLOG(INFO) << "Remote HTTP Server support Range" << std::boolalpha
                 << supportRange << endl;
    }

    std::string file_name;
    if (MUtil::parseFileName(url, file_name)) {
      if (!MUtil::generateDownloadDir(file_name)) {
        DLOG(INFO) << "Create directory failed" << endl;
        continue;
      }
    }

    global_context_ptr->add_file_size_map(file_name, fileSize);
    std::vector<std::future<double>> vf{};
    if (!supportRange) {
      // 访问全局变量
      global_context_ptr->add_file_future_map(
          file_name,
          std::make_pair(
              -1,
              global_context_ptr->getDownloadThreadPool()->enqueue(
                  &DownLoadTask::execute, url, MUtil::getFilePath(file_name))));
    } else {
      using ll = long long;
      ll s = 0, z = s + part_size;

      int part_num = 0;
      while (s < fileSize) {
        string rangeStr = MUtil::generateRange(s, std::min(z, (ll)fileSize));
        DLOG(INFO) << "rangeStr: " << rangeStr << endl;

        //
        global_context_ptr->add_file_future_map(
            file_name,
            std::make_pair(part_num,
                           global_context_ptr->getDownloadThreadPool()->enqueue(
                               &DownLoadTask::execute_for_part, url,
                               MUtil::getFilePath(file_name, part_num),
                               rangeStr)));
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

void waitAndCombine(std::shared_ptr<Context> global_context_ptr,
                    int part_size) {
  int cnt = 0;
  do {
    for (auto &it : global_context_ptr->get_file_future_map()) {
      std::string file_name = it.first;
      CombineTask t{};
      if (!global_context_ptr->get_file_parts(file_name)) {
        // 如果当前下载没有part.
        cnt++;
        continue;
      }

      double fz = global_context_ptr->get_file_size(file_name);
      DLOG(INFO) << "File size: " << fz << endl;
      auto fv = std::move(it.second);
      // 用户存储没有传输正确的结果.
      std::vector<std::pair<int, std::future<double>>> nfv{};
      for (auto &&pair_it_ : fv) {
        int part_index = pair_it_.first;
        int part_num = global_context_ptr->get_file_parts(file_name);
        DLOG(INFO) << "part_num: " << part_num << endl;
        string url = global_context_ptr->file_url_map_[file_name];

        // 使用wait_for是为了获取future_status的状态,可以重试下载功能。从另外一个thread栈中获得数据。
        auto status = pair_it_.second.wait_for(global_context_ptr->wait_time_);
        if (status == std::future_status::ready) {
          // read_size是当前下载线程读到的数据量。
          double read_size = pair_it_.second.get();
          DLOG(INFO) << "read size: " << read_size << endl;

          // part_index是-1的话，代表的是当前是当前这个请求没有分range的
          if (part_index == -1) {
            // 如果数据下载不对，那么需要重传 TODO
            // 可以设置重试下载次数。global_context_ptr 指针是全局可见的，在main
            // thread上存在此变量，实际空间位于堆。由于遍历在main
            // thread上，而且是串行遍历的。不用加锁。
            if (read_size != global_context_ptr->get_file_size(file_name)) {
              nfv.push_back(
                  {part_index,
                   global_context_ptr->getDownloadThreadPool()->enqueue(
                       &DownLoadTask::execute, url,
                       MUtil::getFilePath(file_name))});
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
                   global_context_ptr->getDownloadThreadPool()->enqueue(
                       &DownLoadTask::execute_for_part, url,
                       MUtil::getFilePath(file_name, part_index), rangeStr)});
            }

            // 当前range读取数量不对,或者是最后一个range,但总数不对.
            // 重传不移动future，我们认为此次读取失败.
            else {
              // 如果文件已经下载完毕
              if (global_context_ptr->file_has_read_size_map_[file_name] +
                      read_size ==
                  fz) {
                global_context_ptr->file_has_read_size_map_[file_name] = fz;
                DLOG(INFO) << "END Reading from server " << file_name;
                break;
              } else {
                //
                nfv.push_back(
                    {part_index,
                     global_context_ptr->getDownloadThreadPool()->enqueue(
                         &DownLoadTask::execute_for_part, url,
                         MUtil::getFilePath(file_name, part_index), rangeStr)});
              }
            }
          }
        } else if (status == std::future_status::timeout) {
          if (part_index == -1) {
            // 如果数据下载不对，那么需要重传
            global_context_ptr->add_file_future_map(
                file_name,
                std::make_pair(
                    -1, global_context_ptr->getDownloadThreadPool()->enqueue(
                            &DownLoadTask::execute, url,
                            MUtil::getFilePath(file_name))));
          } else {
            string rangeStr =
                global_context_ptr->file_part_range_map_[file_name][part_index];
            nfv.push_back(
                {part_index,
                 global_context_ptr->getDownloadThreadPool()->enqueue(
                     &DownLoadTask::execute_for_part, url,
                     MUtil::getFilePath(file_name, part_index), rangeStr)});
          }
        } else {
          // 如果future status 为 deferred.
          DLOG(INFO) << "nvf added" << endl;
          nfv.push_back({pair_it_.first, std::move(pair_it_.second)});
        }
      }

      // 如果某个文件的大小与读取数据一致
      if (global_context_ptr->file_has_read_size_map_[file_name] == fz) {
        cnt++;
        DLOG(INFO) << "Read done \n";
        DLOG(INFO) << global_context_ptr->file_has_read_size_map_[file_name]
                   << endl;
        DLOG(INFO) << "Read from server successully" << fz << endl;

        // 利用额外的线程栈进行计算,发送新的task给合并线程。
        global_context_ptr->add_combile_file_future_map(
            file_name,
            std::async(&CombineTask::execute, &t, file_name,
                       global_context_ptr->get_file_parts(file_name)));
        // global_context_ptr->add_combile_file_future_map(
        //     file_name, global_context_ptr->getCombinerThreadPool()->enqueue(
        //                    &CombineTask::execute, &t, file_name,
        //                    global_context_ptr->get_file_parts(file_name)));

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
  // FLAGS_log_dir = "./log";
  google::InitGoogleLogging(argv[0]);
  DLOG(INFO) << "Running" << endl;

  vector<string> urls{};
  int concurrency = 0;
  int part_size = 0;
  MUtil::initOps(urls, concurrency, part_size, argc, argv);
  std::shared_ptr<Context> global_context_ptr =
      std::make_shared<Context>(concurrency);
  downLoad(global_context_ptr->getContext(), urls, part_size);
  waitAndCombine(global_context_ptr->getContext(), part_size);
  return 0;
}