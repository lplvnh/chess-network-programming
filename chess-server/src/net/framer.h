#pragma once
#include <string>
#include <vector>

const size_t MAX_LINE = 512; // độ dài tối đa một dòng, không tính "\r\n"

// Một mẩu byte framer tách ra khỏi bộ đệm, giữ NGUYÊN VĂN (kể cả '\r', '\n') để ghi log
enum class FrameKind
{
  MESSAGE,  // một bản tin hoàn chỉnh, hợp lệ về độ dài → xử lý
  TOO_LONG, // dòng dài quá MAX_LINE → bỏ, trả 400|ERROR|LINE_TOO_LONG
  TAIL      // phần đuôi của dòng quá dài đã báo lỗi trước đó → bỏ, không trả lời thêm
};

struct Frame
{
  FrameKind kind;
  std::string raw;
};

// Lấy hết các mẩu đã đủ ra khỏi buffer, theo đúng thứ tự nhận.
// skipping (lưu trong Client): true nghĩa là đang bỏ dần một dòng quá dài cho tới '\n'.
std::vector<Frame> extractFrames(std::string &buffer, bool &skipping);

// "LOGIN|a|b|RANKED\r\n" → "LOGIN|a|b|RANKED"  (bỏ '\n' cuối và '\r' trước nó nếu có)
std::string stripTerminator(const std::string &raw);
