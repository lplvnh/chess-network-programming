// Auth: đăng ký, đăng nhập, đăng xuất
#pragma once
#include "connection.h"
#include <string>
#include <vector>

void handleRegister(Client &c, const std::vector<std::string> &f);
void handleLogin(Client &c, const std::vector<std::string> &f);
void handleLogout(Client &c, const std::vector<std::string> &f);