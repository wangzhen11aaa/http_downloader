# This tool is for download purpose.
## This tool can split target file into parts, and  to supports concurrently download files into local storage.

# Dependency
1. libcurl library: download files via http protocol only now.
2. glog: log module
3. Popl: to parse the options.

# Usage
    Build:
    Opt 1.
        cmake CMakeLists.txt
        make
    Get Help:
        ./multithreadDownloader -h
    Download Example:
    1. Single download task (recommended)
        ./multithreadDownloader -u https://nodejs.org/dist/v4.2.3/node-v4.2.3-darwin-x64.tar.xz -t 4 --partsize 20000
    2. Multiple download task (Not recommanded)
       Eg. ./multithreadDownloader -u http://localhost:8000/tests/data.txt;https://nodejs.org/dist/v4.2.3/node-v4.2.3-darwin-x64.tar.xz -t 4 --partsize 2000000
