#include "framer.h"

std::vector<std::string> extractLines(std::string &buffer, bool &tooLong)
{
  std::vector<std::string> lines;
  size_t pos;
  while ((pos = buffer.find('\n')) != std::string::npos)
  {
    std::string line = buffer.substr(0, pos);
    buffer.erase(0, pos + 1);
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    lines.push_back(line); // thay vì gọi handleLine, gom vào danh sách
  }
  tooLong = buffer.size() > MAX_LINE;
  if (tooLong)
    buffer.clear();
  return lines;
}