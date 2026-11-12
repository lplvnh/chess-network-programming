// Framer: tách luồng byte TCP thành từng dòng
#pragma once
#include <string>
#include <vector>

const size_t MAX_LINE = 512;

// Lấy hết các dòng đủ '\n' ra khỏi buffer (bỏ '\r' cuối dòng).
// tooLong = true nếu phần còn lại > MAX_LINE mà chưa có '\n' (buffer bị xóa).
std::vector<std::string> extractLines(std::string& buffer, bool& tooLong);

// Cắt "A|B|C" thành {"A","B","C"}
std::vector<std::string> splitFields(const std::string& line);
