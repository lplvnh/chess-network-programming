// Store: danh sách tài khoản người dùng
#pragma once
#include <string>
#include <map>

struct User
{
  std::string name;
  std::string pass;  // tạm lưu nguyên văn, bước 2.5 sẽ đổi thành hash
  int rating = 1200; // rating khởi đầu theo design
  int wins = 0, draws = 0, losses = 0;
};

// Tên user -> thông tin user. "extern" = chỉ khai báo, biến thật nằm ở store.cpp
extern std::map<std::string, User> g_users;