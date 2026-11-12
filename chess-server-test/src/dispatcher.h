// Dispatcher: nhận 1 dòng request, kiểm tra lệnh, gọi hàm xử lý
#pragma once
#include "connection.h"
#include <string>

void dispatch(Client& c, const std::string& line);
