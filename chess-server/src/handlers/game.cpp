#include "game.h"
#include "room.h"
#include "chess.h"

// Kiểm tra dạng UCI: e2e4 hoặc e7e8q
static bool validUciFormat(const std::string &s)
{
  if (s.size() != 4 && s.size() != 5)
    return false;
  if (s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8')
    return false; // ô đi
  if (s[2] < 'a' || s[2] > 'h' || s[3] < '1' || s[3] > '8')
    return false; // ô đến
  if (s.size() == 5 && std::string("qrbn").find(s[4]) == std::string::npos)
    return false;
  return true;
}

// Tên đối thủ của user trong phòng r
static std::string opponentOf(const Room &r, const std::string &user)
{
  return user == r.white ? r.black : r.white;
}

void handleMove(Client &c, const std::vector<std::string> &f)
{
  Room &r = rooms[c.roomId];
  char myColor = (c.user == r.white) ? 'w' : 'b';

  // 1. Đúng lượt chưa?
  if (r.pos.side != myColor)
  {
    sendMsg(c, "407|MOVE|NOT_YOUR_TURN");
    return;
  }
  // 2. Đúng dạng chưa?
  if (f.size() != 2 || !validUciFormat(f[1]))
  {
    sendMsg(c, "400|MOVE|BAD_FORMAT");
    return;
  }
  const std::string &uci = f[1];

  // 3. Có trong danh sách nước hợp lệ không?
  bool found = false;
  Move chosen{};
  for (const Move &m : legalMoves(r.pos))
  {
    if (moveToUci(m) == uci)
    {
      chosen = m;
      found = true;
      break;
    }
  }
  if (!found)
  {
    sendMsg(c, "406|MOVE|ILLEGAL");
    return;
  }

  // 4. Đi quân, rồi báo cho cả hai
  r.pos = makeMove(r.pos, chosen);

  std::string flag = inCheck(r.pos, r.pos.side) ? "CHECK" : "NONE"; // bên SẮP đi có bị chiếu?
  std::string fen = toFen(r.pos);
  sendMsg(c, "200|MOVE|" + uci + "|" + flag + "|" + fen);

  std::string oppName = opponentOf(r, c.user); // thay cho dòng (myColor == 'w') ? ...
  Client *opp = findByUser(oppName);
  if (opp)
    sendMsg(*opp, "OPP_MOVE|" + uci + "|" + flag + "|" + fen);

  // (bước 7: kiểm tra chiếu hết / hòa → GAME_OVER)
}

void handleResign(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 1)
  {
    sendMsg(c, "400|RESIGN|BAD_FORMAT");
    return;
  }
  Room &r = rooms[c.roomId];
  sendMsg(c, "200|RESIGN");                    // response trước
  endGame(r, opponentOf(r, c.user), "RESIGN"); // rồi GAME_OVER cho cả hai
}
// Gọi khi c ngắt kết nối giữa ván: đối thủ thắng
void forfeitOnDisconnect(Client &c)
{
  Room &r = rooms[c.roomId];
  endGame(r, opponentOf(r, c.user), "DISCONNECT", c.user); // không gửi cho c
}