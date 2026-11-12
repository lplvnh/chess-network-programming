#include "logger.h"
#include "connection.h"
#include <cstdio>
#include <ctime>
#include <sys/time.h>

static FILE *logFile = nullptr;

// MỚI: hàm mở file, thay cho dòng fopen trong main()
void logOpen(const std::string &path)
{
  logFile = fopen(path.c_str(), "a");
}

// Trả về giờ hiện tại dạng "2026-11-12 23:28:17.779"
static std::string now()
{
  timeval tv;
  gettimeofday(&tv, nullptr); // lấy giờ (giây + micro giây)
  tm t;
  localtime_r(&tv.tv_sec, &t); // đổi sang giờ địa phương
  char buf[64];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
  char out[80];
  snprintf(out, sizeof(out), "%s.%03ld", buf, (long)(tv.tv_usec / 1000)); // thêm mili giây
  return out;
}

// tag: CONN / RECV / SEND / DISC
void logMsg(const std::string &tag, const Client &c, const std::string &text)
{
  std::string line = now() + " " + tag + " fd=" + std::to_string(c.fd) +
                     " " + c.addr + " : " + text;
  printf("%s\n", line.c_str()); // ra terminal
  fflush(stdout);
  if (logFile)
  {
    fprintf(logFile, "%s\n", line.c_str()); // ra file
    fflush(logFile);                        // ghi xuống đĩa ngay, server chết đột ngột vẫn còn log
  }
}