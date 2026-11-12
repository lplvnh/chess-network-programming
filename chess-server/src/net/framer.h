#pragma once
#include <string>
#include <vector>

const size_t MAX_LINE = 512;

// Lấy hết các dòng hoàn chỉnh ra khỏi buffer.
// tooLong = true nếu phần còn lại dài quá MAX_LINE (khi đó buffer bị xóa)
std::vector<std::string> extractLines(std::string &buffer, bool &tooLong);