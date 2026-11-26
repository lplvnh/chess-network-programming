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
  char out[96];
  snprintf(out, sizeof(out), "%s.%03ld", buf, (long)(tv.tv_usec / 1000)); // thêm mili giây
  return out;
}

// Hiển thị từng byte: ký tự in được giữ nguyên, byte điều khiển viết dạng escape.
// Đây chỉ là cách HIỂN THỊ; nội dung và thứ tự byte không thay đổi.
static std::string visible(const std::string &raw)
{
  std::string out;
  for (unsigned char ch : raw)
  {
    if (ch == '\n') out += "\\n";
    else if (ch == '\r') out += "\\r";
    else if (ch == '\t') out += "\\t";
    else if (ch == '\\') out += "\\\\";
    else if (ch < 0x20 || ch >= 0x7F)
    {
      char hex[8];
      snprintf(hex, sizeof(hex), "\\x%02X", ch);
      out += hex;
    }
    else out += (char)ch;
  }
  return out;
}

static void writeLine(const std::string &line)
{
  printf("%s\n", line.c_str()); // ra terminal
  fflush(stdout);
  if (logFile)
  {
    fprintf(logFile, "%s\n", line.c_str()); // ra file
    fflush(logFile);                        // ghi xuống đĩa ngay, server chết đột ngột vẫn còn log
  }
}

static std::string prefix(const std::string &tag, const Client &c)
{
  return now() + " " + tag + " fd=" + std::to_string(c.fd) + " " + c.addr +
         " user=" + (c.user.empty() ? "-" : c.user);
}

void logMsg(const std::string &tag, const Client &c, const std::string &raw)
{
  writeLine(prefix(tag, c) + " " + std::to_string(raw.size()) + "B : " + visible(raw));
}

void logEvent(const std::string &tag, const Client &c, const std::string &note)
{
  writeLine(prefix(tag, c) + " : " + note);
}
