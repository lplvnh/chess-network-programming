#include "logger.h"
#include <cstdio>
#include <ctime>
#include <sys/time.h>

static FILE* g_file = nullptr;

bool Logger::open(const std::string& path) {
    g_file = fopen(path.c_str(), "a");
    return g_file != nullptr;
}

// Thời gian dạng 2026-11-05 14:03:07.512
static std::string now() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    tm t;
    localtime_r(&tv.tv_sec, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    char out[64];
    snprintf(out, sizeof(out), "%s.%03ld", buf, (long)(tv.tv_usec / 1000));
    return out;
}

void Logger::log(const std::string& tag, int fd, const std::string& addr,
                 const std::string& user, const std::string& text) {
    std::string line = now() + " " + tag + " fd=" + std::to_string(fd) + " " + addr +
                       " user=" + (user.empty() ? "-" : user) + " : " + text;
    printf("%s\n", line.c_str());          // terminal
    fflush(stdout);
    if (g_file) {                           // file
        fprintf(g_file, "%s\n", line.c_str());
        fflush(g_file);                     // server chết đột ngột vẫn còn log
    }
}
