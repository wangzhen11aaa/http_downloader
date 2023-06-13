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
class DownLoadTask {
public:
  DownLoadTask() = default;
  ~DownLoadTask() { DLOG(INFO) << "DownLoadTask has freed\n"; }
  double execute(const string &url, string partfileName);

  double execute_for_part(const string &url, string partfileName,
                          string &rangeStr);

  static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
  }
};

class CombineTask {
public:
  double execute(const string &filename, int parts);
  CombineTask() = default;
  ~CombineTask() {
    DLOG(INFO) << "CombineTask destroyed \n";
    // fclose(rfPtr_);
    // fclose(wfPtr_);
  }

private:
  FILE *wfPtr_;
  FILE *rfPtr_;
};
