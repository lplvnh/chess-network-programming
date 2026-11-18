#include "room.h"
#include "store.h"
#include <random>
#include <cmath>     // std::pow, std::lround
#include <algorithm> // std::max

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
                  opp + "|" + std::to_string(oppRating) + "|" + toFen(r.pos));
}

std::string newRoomId()
{
  return "R" + std::to_string(nextRoomNumber++);
}

// Bắt đầu ván trong phòng r (đã có sẵn) giữa a và b
void beginGame(Room &r, Client &a, Client &b)
{
  r.state = "PLAYING";
  parseFen(START_FEN, r.pos);

  r.seen[positionKey(r.pos)] = 1; // thế cờ ban đầu = lần thứ nhất

  Client *white = &a;
  Client *black = &b;
  if (coinFlip())
    std::swap(white, black);
  r.white = white->user;
  r.black = black->user;

  for (Client *c : {white, black})
  {
    c->state = ConnState::IN_GAME;
    c->roomId = r.id;
  }
  sendGameStart(*white, r, "WHITE", r.black);
  sendGameStart(*black, r, "BLACK", r.white);
}

// Ghép trận: tạo phòng mới rồi bắt đầu luôn
void startGame(Client &a, Client &b, const std::string &source)
{
  Room r;
  r.id = newRoomId();
  r.source = source;
  rooms[r.id] = r;
  beginGame(rooms[r.id], a, b); // truyền phòng THẬT trong map, không phải bản sao r
}

// Δ cho người có rating myRating gặp oppRating, đạt score (1 / 0.5 / 0)
static int eloDelta(int myRating, int oppRating, double score)
{
  double expected = 1.0 / (1.0 + std::pow(10.0, (oppRating - myRating) / 400.0));
  return (int)std::lround(32 * (score - expected));
}

// 14 → "+14", -14 → "-14", 0 → "+0"
static std::string signedNumber(int x)
{
  return (x >= 0 ? "+" : "") + std::to_string(x);
}

void endGame(Room &r, const std::string &winner, const std::string &reason,
             const std::string &absentUser)
{
  User &w = g_users[r.white];
  User &b = g_users[r.black];

  // 1. Elo: tính một lần theo góc nhìn bên trắng
  double whiteScore = winner.empty() ? 0.5 : (winner == r.white ? 1.0 : 0.0);
  int delta = eloDelta(w.rating, b.rating, whiteScore);
  int oldW = w.rating, oldB = b.rating;
  w.rating = std::max(100, w.rating + delta); // sàn 100
  b.rating = std::max(100, b.rating - delta);

  if (winner.empty())
  {
    w.draws++;
    b.draws++;
  }
  else if (winner == r.white)
  {
    w.wins++;
    b.losses++;
  }
  else
  {
    b.wins++;
    w.losses++;
  }
  storeSave(); // ghi users.txt ngay

  // 2 + 3. Gửi GAME_OVER cho từng người, đưa về IDLE
  for (const std::string &name : {r.white, r.black})
  {
    Client *c = findByUser(name);
    if (c == nullptr)
      continue; // đã offline
    if (name != absentUser)
    {
      std::string result = winner.empty() ? "DRAW" : (winner == name ? "WIN" : "LOSS");
      User &u = g_users[name];
      int change = u.rating - (name == r.white ? oldW : oldB); // thay đổi THỰC TẾ (đã tính sàn 100)
      sendMsg(*c, "GAME_OVER|" + result + "|" + reason + "|" +
                      std::to_string(u.rating) + "|" + signedNumber(change));
    }
    c->state = ConnState::IDLE;
    c->roomId.clear();
  }
  rooms.erase(r.id); // hủy phòng (sau dòng này KHÔNG dùng r nữa)
}