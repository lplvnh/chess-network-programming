// Auth: đăng ký, đăng nhập, đăng xuất
#pragma once
#include "connection.h"
#include <string>
#include <vector>

void handleRegister(Client &c, const std::vector<std::string> &f);