#include "Task.h"

#include <cstdio>
#include <exception>

#include "MUtil.h"

namespace DownLoadTask {

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

double execute(const string &url, string partfileName) {
  CURL *curl_handle;
  const char *pagefilename = partfileName.c_str();
  FILE *pagefile;

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* set URL to get here */
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

  /* Switch on full protocol/debug output while testing */
  curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

  /* disable progress meter, set to 0L to enable it */
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

  /* open the file */
  pagefile = fopen(pagefilename, "wb");

  if (pagefile) {
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

    /* get it! */
    curl_easy_perform(curl_handle);

    /* close the header file */
    fclose(pagefile);
  }
  double ds;
  curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD, &ds);
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  curl_global_cleanup();

  return ds;
}

double execute_for_part(const string &url, string partfileName,
                        string &rangeStr) {
  CURL *curl_handle;
  const char *pagefilename = partfileName.c_str();
  FILE *pagefile;

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* set URL to get here */
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

  /* Switch on full protocol/debug output while testing */
  curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

  /* disable progress meter, set to 0L to enable it */
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);

  curl_easy_setopt(curl_handle, CURLOPT_RANGE, (rangeStr.c_str()));
  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
                   DownLoadTask::write_data);

  /* open the file */
  pagefile = fopen(pagefilename, "wb");

  if (pagefile) {
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

    /* get it! */
    curl_easy_perform(curl_handle);

    /* close the header file */
    fclose(pagefile);
  }
  double ds;
  curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD, &ds);
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  curl_global_cleanup();

  return ds;
}
}  // namespace DownLoadTask

// 如果使用RAII机制(即使用class这种机制)(资源分配/使用/释放
// 是安全的，即用来保证不会造成内存泄漏)，内存泄漏一般发生在堆上内存的泄漏，或者资源(文件比如Socket未能及时关闭而导致内存泄漏)未能成功释放，而栈上的内存资源是一定会随着函数栈释放，但是栈上引用的其他的资源可能会因为不能通过栈上的handle释放此进程/线程的管理的资源而造成内存泄漏。
// 需要极度关注的是堆上内存的管理(分配/使用/释放)及其应该管理资源的绝对正确释放。
// 这里的CombineTask因为包含了对于某些管理资源的变量比如 wfPtr_以及
// rfPtr_都是管理资源的变量。
double CombineTask::execute(const string &filename, int parts) {
  try {
    double ret = 0.0;
    char rwbuffer[2048];

    string partFilePath;
    wf.open(MUtil::getFilePath(filename).c_str(), std::ios::out);

    for (int i = 0; i < parts; i++) {
      partFilePath = MUtil::getFilePath(filename, i);
      rf = std::fstream(partFilePath.c_str(), std::ios::in);
      // rf.open(partFilePath.c_str(), std::ios_base::in);

      // 获取当前文件的大小
      long long fz = MUtil::getFileSize(rf);
      DLOG(INFO) << "File size: " << fz << endl;
      double hasReadSize = 0.0;
      while (hasReadSize < fz) {
        rf.read(rwbuffer, 2048);

        wf.write(rwbuffer, sizeof(rwbuffer));

        hasReadSize += rf.gcount();

        DLOG(INFO) << "hasReadSize: " << hasReadSize << endl;
      }

      rf.close();
      remove(partFilePath.c_str());
      ret += hasReadSize;
    }
    return ret;
    wf.close();
  } catch (const std::exception &e) {
    DLOG(ERROR) << e.what() << endl;
    exit(1);
  }
  return 0.0;
}

CombineTask::~CombineTask() {
  if (rf.is_open()) {
    rf.close();
  }
  if (wf.is_open()) {
    wf.close();
  }
  DLOG(INFO) << "CombineTask destroyed \n";
}