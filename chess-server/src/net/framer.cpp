#include "framer.h"

std::vector<Frame> extractFrames(std::string &buffer, bool &skipping)
{
  std::vector<Frame> frames;
  size_t pos;
  while ((pos = buffer.find('\n')) != std::string::npos)
  {
    std::string raw = buffer.substr(0, pos + 1); // giữ nguyên '\n' để log đúng byte nhận
    buffer.erase(0, pos + 1);
    if (skipping)
    { // đuôi của dòng quá dài: đã trả lỗi rồi, chỉ bỏ đi
      frames.push_back({FrameKind::TAIL, raw});
      skipping = false;
    }
    else if (stripTerminator(raw).size() > MAX_LINE)
      frames.push_back({FrameKind::TOO_LONG, raw}); // dòng đủ '\n' nhưng quá dài
    else
      frames.push_back({FrameKind::MESSAGE, raw});
  }
  // Chưa thấy '\n' mà đã quá dài (+1 chừa chỗ cho '\r' của "\r\n" chưa tới): bỏ ngay để
  // bộ đệm không phình, phần còn lại của dòng này sẽ bị bỏ tiếp cho tới '\n'
  if (buffer.size() > MAX_LINE + 1)
  {
    frames.push_back({skipping ? FrameKind::TAIL : FrameKind::TOO_LONG, buffer});
    buffer.clear();
    skipping = true;
  }
  return frames;
}

std::string stripTerminator(const std::string &raw)
{
  std::string line = raw;
  if (!line.empty() && line.back() == '\n')
    line.pop_back();
  if (!line.empty() && line.back() == '\r')
    line.pop_back();
  return line;
}
