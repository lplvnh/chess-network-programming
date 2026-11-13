#include "lobby.h"
#include "store.h"

// Đổi trạng thái kết nối sang chữ hiển thị theo design
static std::string statusName(ConnState s)
{
  switch (s)
  {
  case ConnState::IDLE:
    return "IDLE";
  case ConnState::QUEUED:
    return "QUEUED";
  case ConnState::ROOM_WAITING:
    return "ROOM";
  case ConnState::IN_GAME:
    return "INGAME";
  default:
    return ""; // CONNECTED: không bao giờ tới đây
  }
}

void handleListOnline(Client &c, const std::vector<std::string> &)
{
  // 1. Gom các dòng USER
  std::vector<std::string> rows;
  for (auto &[fd, other] : clients)
  {
    if (other.user.empty())
      continue;                              // chưa đăng nhập → bỏ qua
    int rating = g_users[other.user].rating; // rating nằm trong User, không nằm trong Client
    rows.push_back("USER|" + other.user + "|" + std::to_string(rating) + "|" +
                   other.mode + "|" + statusName(other.state));
  }

  // 2. Dòng đầu (có số n) + các dòng USER → gửi cả khối
  std::vector<std::string> lines;
  lines.push_back("200|LIST_ONLINE|" + std::to_string(rows.size()));
  for (const std::string &r : rows)
    lines.push_back(r);
  sendBlock(c, lines);
}