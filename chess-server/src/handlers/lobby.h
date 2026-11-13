// Lobby: xem người online, bảng xếp hạng
#pragma once
#include "connection.h"
#include <string>
#include <vector>

void handleListOnline(Client &c, const std::vector<std::string> &f);
void handleLeaderboard(Client &c, const std::vector<std::string> &f); // ← THÊM