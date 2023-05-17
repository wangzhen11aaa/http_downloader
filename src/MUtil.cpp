#include "MUtil.h"

namespace MUtil {
// 从url中获取文件名

bool parseFileName(const string &url, string &filename) {
  auto p = url.rfind("/");
  if (p != std::string::npos) {
    filename = url.substr(p + 1);
    DLOG(INFO) << "parseFileName: " << filename << endl;
    return true;
  }
  return false;
}

bool generateDownloadDir(const string &filename) {
  int n = filename.size();
  string downlaodDir = filename.substr(0, n > 3 ? 3 : 0) + "_download";
  if (fs::exists(downlaodDir)) {
    DLOG(INFO) << "DownloadDir exists" << endl;
    return true;
  }
  try {
    if (fs::create_directory(fs::path{downlaodDir.c_str()})) {
      DLOG(INFO) << "Create Directory " << downlaodDir << "successfully"
                 << endl;
    }
  } catch (const exception &e) {
    DLOG(INFO) << "Create Directory " << downlaodDir << "failed";
    return false;
  }
  return true;
}

string getFilePath(const string &filename) {
  int n = filename.size();
  auto pathName =
      filename.substr(0, n > 3 ? 3 : 0) + "_download" + "/" + filename;
  DLOG(INFO) << "getFilePath: " << pathName << endl;
  return pathName;
}

string getFilePath(const string &filename, int i) {

  auto pathName = filename.substr(0, filename.size() > 3 ? 3 : 0) +
                  "_download" + "/" + filename + "_" + to_string(i);
  DLOG(INFO) << "getFilePath: " << pathName << endl;
  return pathName;
}

void splitUrl(const string &urls, vector<string> &url_vec) {
  if (urls.find(';') != string::npos) {
    int s = 0;
    int i = 0;
    for (; i < urls.size(); i++) {
      if (urls[i] == ';') {
        url_vec.push_back(urls.substr(s, i));
        s = i + 1;
      }
    }
    url_vec.push_back(urls.substr(s, i));
  } else {
    url_vec.push_back(urls);
  }
}

string generateRange(long long s, long long e) {
  return to_string(s) + "-" + to_string(e);
}

long long getFileSize(FILE *f) {
  long long sz = 0;
  fseek(f, 0, SEEK_END);
  sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  return sz;
}

void initOps(vector<string> &url_v, int &concurrency, int &part_size, int argc,
             char **argv) {
  OptionParser op("Allowed Options");
  auto help_option = op.add<Switch>("h", "help", "produce help message");
  auto urls_option =
      op.add<Value<string>>("u", "urls", "URL for the resource, split by ;");
  auto part_size_option =
      op.add<Value<int>>("", "partsize", "File Part size", 1024 * 1024);
  auto concurrency_option = op.add<Value<int>>(
      "t", "threads", "Concurrency number of threads for download", 4);
  op.parse(argc, argv);
  if (help_option->count() == 1) {
    DLOG(INFO) << op << endl;
  } else if (help_option->count() == 2)
    DLOG(INFO) << op.help(Attribute::advanced) << "\n";
  else if (help_option->count() > 2)
    DLOG(INFO) << op.help(Attribute::expert) << "\n";
  if (!urls_option->is_set()) {
    DLOG(INFO) << "需要输入url";
    DLOG(INFO) << op << endl;
    exit(1);
  }
  string urls = urls_option->value();
  DLOG(INFO) << "urls:" << urls << endl;
  MUtil::splitUrl(urls, url_v);
  for (auto &url : url_v) {
    DLOG(INFO) << "url: " << url << endl;
  }
  concurrency = concurrency_option->value();
  DLOG(INFO) << "concurrency:" << concurrency << endl;
  part_size = part_size_option->value();
  DLOG(INFO) << "part_size:" << part_size << endl;
}

double computeDownLoadSpeed(double delta_seconds, double delta_size) {
  assert(delta_seconds != 0);
  return delta_size / delta_seconds;
}
}; // namespace MUtil