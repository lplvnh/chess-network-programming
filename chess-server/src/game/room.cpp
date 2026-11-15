#include "room.h"
#include "store.h"
#include <random>

std::map<std::string, Room> rooms;
static int nextRoomNumber = 1;

// Tung đồng xu: true hoặc false, mỗi bên 50%
static bool coinFlip()
{
  static std::mt19937 rng(std::random_device{}()); // bộ sinh số ngẫu nhiên, chỉ tạo 1 lần
  return rng() % 2 == 0;
}

// Gửi GAME_START cho MỘT người (màu của họ + thông tin đối thủ)
static void sendGameStart(Client &me, const Room &r, const std::string &color,
                          const std::string &opp)
{
  int oppRating = g_users[opp].rating;
  sendMsg(me, "GAME_START|" + r.id + "|" + r.source + "|" + color + "|" +
                  opp + "|" + std::to_string(oppRating) + "|" + r.fen);
}

void startGame(Client &a, Client &b, const std::string &source)
{
  Room r;
  r.id = "R" + std::to_string(nextRoomNumber++); // R1, R2, ...
  r.source = source;
  r.fen = START_FEN;

  // Bốc màu: mặc định a trắng, tung đồng xu thì đổi
  Client *white = &a;
  Client *black = &b;
  if (coinFlip())
    std::swap(white, black);
  r.white = white->user;
  r.black = black->user;
  rooms[r.id] = r;

  // Cả hai vào trạng thái đang chơi
  for (Client *c : {white, black})
  {
    c->state = ConnState::IN_GAME;
    c->roomId = r.id;
  }

  // Mỗi người một bản tin riêng
  sendGameStart(*white, r, "WHITE", r.black);
  sendGameStart(*black, r, "BLACK", r.white);
}