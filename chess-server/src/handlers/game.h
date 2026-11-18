// Game: các lệnh trong ván cờ (đi quân; sau này: xin thua, cầu hòa)
#pragma once
#include "connection.h"
#include <string>
#include <vector>

void handleMove(Client &c, const std::vector<std::string> &f);