// Rooms: tạo phòng, mời, chấp nhận/từ chối, rời phòng
#pragma once
#include "connection.h"
#include <string>
#include <vector>

void handleCreateRoom(Client &c, const std::vector<std::string> &f);
void handleInvite(Client &c, const std::vector<std::string> &f);
void handleAcceptInvite(Client &c, const std::vector<std::string> &f);  // bước 5.2
void handleDeclineInvite(Client &c, const std::vector<std::string> &f); // bước 5.2
void handleLeaveRoom(Client &c, const std::vector<std::string> &f);

void cancelInvitesTo(Client &c); // bước 5.2: hủy mọi lời mời TỚI c
void closeRoom(Client &host);    // hủy phòng đang chờ của chủ phòng
void roomTick();                 // bước 5.3: dọn lời mời hết hạn