// Matchmaker: hàng đợi tìm trận
#pragma once
#include "connection.h"
#include <string>
#include <vector>

void handleFindMatch(Client &c, const std::vector<std::string> &f);
void handleCancelMatch(Client &c, const std::vector<std::string> &f);
void leaveQueue(Client &c); // dùng chung cho CANCEL, LOGOUT, ngắt kết nối
void matchmakerTick();