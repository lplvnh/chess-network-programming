#include "lobby.h"
#include "store.h"
#include <algorithm>

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

// "10" → n=10, true.   "abc", "0", "51", "" → false
static bool parseCount(const std::string &s, int &n)
{
  if (s.empty() || s.size() > 2)
    return false; // tối đa 2 chữ số
  for (char ch : s)
    if (ch < '0' || ch > '9')
      return false; // chỉ chấp nhận chữ số
  n = std::stoi(s);
  return n >= 1 && n <= 50;
}

// true nếu a phải đứng TRƯỚC b trong bảng xếp hạng
static bool rankBefore(const User *a, const User *b)
{
  if (a->rating != b->rating)
    return a->rating > b->rating; // 1. rating cao trước
  if (a->wins != b->wins)
    return a->wins > b->wins; // 2. thắng nhiều trước
  return a->name < b->name;   // 3. tên a→z
}

void handleLeaderboard(Client &c, const std::vector<std::string> &f)
{
  // 1. Đọc n (không có thì mặc định 10)
  int n = 10;
  if (f.size() > 2 || (f.size() == 2 && !parseCount(f[1], n)))
  {
    sendMsg(c, "400|LEADERBOARD|BAD_FORMAT");
    return;
  }

  // 2. Gom con trỏ tới mọi user rồi sắp xếp
  std::vector<const User *> sorted;
  for (const auto &[name, u] : g_users)
    sorted.push_back(&u);
  std::sort(sorted.begin(), sorted.end(), rankBefore);

  // 3. Tìm hạng của người gửi
  int myRank = 0;
  for (size_t i = 0; i < sorted.size(); i++)
    if (sorted[i]->name == c.user)
      myRank = i + 1; // hạng bắt đầu từ 1
  int myRating = g_users[c.user].rating;

  // 4. Dòng đầu + k dòng RANK
  int k = std::min<int>(n, sorted.size()); // không đủ n người thì lấy hết
  std::vector<std::string> lines;
  lines.push_back("200|LEADERBOARD|" + std::to_string(k) + "|" +
                  std::to_string(myRank) + "|" + std::to_string(myRating));
  for (int i = 0; i < k; i++)
  {
    const User *u = sorted[i];
    lines.push_back("RANK|" + std::to_string(i + 1) + "|" + u->name + "|" +
                    std::to_string(u->rating) + "|" + std::to_string(u->wins) + "|" +
                    std::to_string(u->draws) + "|" + std::to_string(u->losses));
  }
  sendBlock(c, lines); // dùng lại hàm của bước 3.1
}