#include "HTTPDownLoadHelper.h"
#include "glog/logging.h"

HTTPDownLoadHelper *HTTPDownLoadHelper::instance_ = nullptr;

void HTTPDownLoadHelper::getHTTPRemoteFileSize(bool &remote_support_range,
                                               const string &url_str,
                                               double &file_size) {
  CURL *curl = curl_easy_init();
  if (curl) {
    // 先获取整体文件大小
    curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    //auto res = curl_easy_perform(curl);

    int res_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
    DLOG(INFO) << "Response code: " << res_code << endl;
    // 获取文件大小
    double cnt;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cnt);
    if (res_code == 200) {
      DLOG(INFO) << "File size: " << cnt;
      file_size = cnt;
    } else {
      DLOG(INFO) << "Request failed" << endl;
      file_size = 0;
    }

    curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");
    //res = curl_easy_perform(curl);
    // 通过再次访问获得此文件是否支持range.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
    remote_support_range = res_code == 206 ? true : false;
    curl_easy_cleanup(curl);
    /* Perform the request */
  }
}
