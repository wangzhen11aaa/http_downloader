#ifndef MUTIL_H
#define MUTIL_H
#endif

#include "Popl.h"
#include "glog/logging.h"
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

namespace fs = std::filesystem;
using popl::Attribute;
using popl::OptionParser;
using popl::Switch;
using popl::Value;
using std::endl;
using std::exception;
using std::string;
using std::to_string;
using std::vector;
namespace MUtil {
bool parseFileName(const string &url, string &filename);
bool generateDownloadDir(const string &filename);
string getFilePath(const string &filename);
string getFilePath(const string &filename, int i);
void splitUrl(const string &urls, vector<string> &url_vec);
string generateRange(long long s, long long e);
long long getFileSize(FILE *f);
void initOps(vector<string> &url_v, int &concurrency, int &part_size, int argc,
             char **argv);

// Bytes / second.
double computeDownLoadSpeed(double delta_seconds, double delta_size); 
} // namespace MUtil
