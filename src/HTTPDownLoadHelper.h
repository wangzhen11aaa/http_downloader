#ifndef HTTPDOWNLOADHELPER_H
#define HTTPDOWNLOADHELPER_H
#endif

#include <curl/curl.h>
#include <iostream>

namespace http_download {
// 单例模式
class HTTPDownLoadHelper {
public:
  static HTTPDownLoadHelper *getInstance() {
    if (instance_) {
      return instance_;
    } else {
      instance_ = new HTTPDownLoadHelper();
      return instance_;
    }
  }

  // 获取
  void getHTTPRemoteFileSize(bool &remote_support_range,
                             const std::string &url_str, double &file_size);

private:
  static HTTPDownLoadHelper *instance_;
};
}; // namespace http_download