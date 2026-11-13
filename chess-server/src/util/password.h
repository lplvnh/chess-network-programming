// Hash và kiểm tra mật khẩu (SHA-256 + salt, dùng crypt() của Linux)
#pragma once
#include <string>

std::string hashPassword(const std::string &pass);                      // dùng khi đăng ký
bool checkPassword(const std::string &pass, const std::string &stored); // dùng khi đăng nhập