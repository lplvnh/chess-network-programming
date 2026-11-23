#pragma once
#include <string>

struct Client; // "có một struct tên Client", không cần include cả connection.h

void logOpen(const std::string &path);

// Ghi một bản tin: tag = RECV / SEND / DROP, raw = đúng các byte đã nhận hoặc sắp gửi
// (kể cả '\n'). Không che, không cắt, không ghép. Byte điều khiển được hiển thị dạng
// \n \r \t \xHH và '\' thành "\\" để nhìn thấy được mà vẫn khôi phục đúng từng byte.
void logMsg(const std::string &tag, const Client &c, const std::string &raw);

// Ghi một sự kiện kết nối: tag = CONN / DISC, note = mô tả
void logEvent(const std::string &tag, const Client &c, const std::string &note);
