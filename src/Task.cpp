#include "Task.h"
#include "MUtil.h"
#include <cstdio>
#include <exception>

namespace DownLoadTask{

 size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

double execute(const string& url, string partfileName){
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
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
                   write_data);

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
}


double CombineTask::execute(const string& filename, int parts) {
  try {
    double ret = 0.0;
    char rwbuffer[2048];
    wfPtr_ = fopen(MUtil::getFilePath(filename).c_str(), "w+");
    string partFilePath;
    DLOG(INFO) << "Open for write";
    for (int i = 0; i < parts; i++) {
      partFilePath = MUtil::getFilePath(filename, i);
      rfPtr_ = fopen(partFilePath.c_str(), "r");
      long long fz = MUtil::getFileSize(rfPtr_);
      DLOG(INFO) << "File size: " << fz << endl;
      double hasReadSize = 0.0;
      while (hasReadSize < fz) {
        auto rret =
            fread(rwbuffer, sizeof(rwbuffer[0]), sizeof(rwbuffer), rfPtr_);
        DLOG(INFO) << "rret: " << rret << endl;
        auto wret = fwrite(rwbuffer, sizeof(rwbuffer[0]), rret, wfPtr_);
        DLOG(INFO) << "wret: " << wret << endl;
        hasReadSize += wret;
        DLOG(INFO) << "hasReadSize: " << hasReadSize << endl;
      }
      fclose(rfPtr_);
      remove(partFilePath.c_str());
      ret += hasReadSize;
    }
    return ret;
    fclose(wfPtr_);
  } catch (const std::exception &e) {
    DLOG(ERROR) << e.what() << endl;
    exit(1);
  }
  return 0.0;
  }