// Room: một ván cờ giữa 2 người (bước sau sẽ thêm luật cờ, cầu hòa...)
#pragma once
#include "connection.h"
#include <string>
#include <map>
#include <ctime>
#include "chess.h"

// Thế cờ ban đầu theo chuẩn FEN
const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

struct Room
{
  std::string id;                        // "R1", "R2"...
  std::string source;                    // RANDOM / RANKED / ROOM: ván được tạo bằng cách nào
  std::string state;                     // ← THÊM: "WAITING" hoặc "PLAYING"
  std::string host;                      // ← THÊM: chủ phòng (chỉ có với source ROOM)
  std::string white;                     // tên người cầm trắng
  std::string black;                     // tên người cầm đen
  Position pos;                          // thế cờ hiện tại
  std::map<std::string, int> seen;       // positionKey → số lần xuất hiện
  std::map<std::string, time_t> invites; // ← THÊM: tên người được mời → hạn chót
};

extern std::map<std::string, Room> rooms; // mã phòng → Room

std::string newRoomId();
void beginGame(Room &r, Client &a, Client &b);

// Tạo phòng cho a và b, bốc màu, gửi GAME_START cho cả hai
void startGame(Client &a, Client &b, const std::string &source);

// Kết thúc ván: tính Elo, lưu file, gửi GAME_OVER, hủy phòng.
// winner = "" nghĩa là hòa. absentUser = người vừa ngắt kết nối (không gửi cho họ).
void endGame(Room &r, const std::string &winner, const std::string &reason,
             const std::string &absentUser = "");