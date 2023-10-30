#ifndef Task_H
#define Task_H
#endif
#include "glog/logging.h"
#include <curl/curl.h>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
//#include <util.h>
using std::endl;
using std::string;


class CombineTask {
public:
  double execute(const string &filename, int parts);
  CombineTask() = default;
  ~CombineTask() {
    DLOG(INFO) << "CombineTask destroyed \n";
  }

private:
  FILE *wfPtr_;
  FILE *rfPtr_;
};
namespace DownLoadTask {

  double execute(const string &url, string partfileName);

  double execute_for_part(const string &url, string partfileName,
                          string &rangeStr);

   size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);

}