#ifndef HTTPDOWNLOADHELPER_H
#define HTTPDOWNLOADHELPER_H
#endif

#include <curl/curl.h>
#include <iostream>
#include <memory>
#include <mutex>

namespace http_download {
// 单例模式
class HTTPDownLoadHelper {
public:
  static HTTPDownLoadHelper *getInstance() {
    if (!instance_) {
      std::lock_guard<std::mutex> lk(mutex_);
      if (!instance_) {
        return new HTTPDownLoadHelper();
      }
    }
    return instance_;
  }

  // 获取
  void getHTTPRemoteFileSize(bool &remote_support_range,
                             const std::string &url_str, double &file_size);

private:
  static HTTPDownLoadHelper *instance_;
  static std::mutex mutex_;
};
}; // namespace http_download