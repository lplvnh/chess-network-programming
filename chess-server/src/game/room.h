// Room: một ván cờ giữa 2 người (bước sau sẽ thêm luật cờ, cầu hòa...)
#pragma once
#include "connection.h"
#include <string>
#include <map>

// Thế cờ ban đầu theo chuẩn FEN
const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

struct Room
{
  std::string id;     // "R1", "R2"...
  std::string source; // RANDOM / RANKED / ROOM: ván được tạo bằng cách nào
  std::string white;  // tên người cầm trắng
  std::string black;  // tên người cầm đen
  std::string fen;    // thế cờ hiện tại
};

extern std::map<std::string, Room> rooms; // mã phòng → Room

// Tạo phòng cho a và b, bốc màu, gửi GAME_START cho cả hai
void startGame(Client &a, Client &b, const std::string &source);