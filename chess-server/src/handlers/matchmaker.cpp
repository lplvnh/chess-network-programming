#include "matchmaker.h"
#include "room.h"
#include "rooms.h"
#include <deque>
#include <algorithm>
#include <cstdlib> // std::abs (trị tuyệt đối)
#include "store.h" // g_users (cần rating)

static std::deque<int> randomQueue; // fd của người chờ ở chế độ RANDOM
static std::deque<int> rankedQueue; // fd của người chờ ở chế độ RANKED (dùng ở 4.2)

// Chọn đúng hàng đợi theo chế độ đã chọn lúc LOGIN
static std::deque<int> &queueOf(const Client &c)
{
  return c.mode == "RANDOM" ? randomQueue : rankedQueue;
}

// Còn từ 2 người trở lên thì lấy 2 người ĐẦU hàng (chờ lâu nhất) ghép với nhau
static void matchRandom()
{
  while (randomQueue.size() >= 2)
  {
    int fdA = randomQueue.front();
    randomQueue.pop_front();
    int fdB = randomQueue.front();
    randomQueue.pop_front();
    startGame(clients[fdA], clients[fdB], "RANDOM");
  }
}

// Khoảng rating chấp nhận của c, tùy thời gian đã chờ
static int ratingRange(const Client &c, time_t now)
{
  int waited = now - c.queuedAt;                  // số giây đã chờ
  return std::min(100 + 50 * (waited / 10), 400); // chia số nguyên: 19/10 = 1
}

static void matchRanked()
{
  time_t now = time(nullptr);
  bool matched = true;
  while (matched)
  { // ghép được 1 cặp thì quét lại từ đầu
    matched = false;
    // Hàng đợi xếp theo thứ tự vào → i nhỏ = chờ lâu nhất
    for (size_t i = 0; i < rankedQueue.size() && !matched; i++)
    {
      Client &a = clients[rankedQueue[i]];
      int ra = g_users[a.user].rating;

      // Tìm người hợp lệ có chênh lệch nhỏ nhất
      int bestJ = -1, bestDiff = 0;
      for (size_t j = 0; j < rankedQueue.size(); j++)
      {
        if (j == i)
          continue; // không tự ghép với mình
        Client &b = clients[rankedQueue[j]];
        int diff = std::abs(ra - g_users[b.user].rating);
        int allowed = std::min(ratingRange(a, now), ratingRange(b, now));
        if (diff <= allowed && (bestJ == -1 || diff < bestDiff))
        {
          bestJ = j;
          bestDiff = diff;
        }
      }

      if (bestJ != -1)
      {
        int fdA = rankedQueue[i];
        int fdB = rankedQueue[bestJ];
        // Xóa vị trí LỚN trước, để vị trí nhỏ không bị lệch
        rankedQueue.erase(rankedQueue.begin() + std::max<int>(i, bestJ));
        rankedQueue.erase(rankedQueue.begin() + std::min<int>(i, bestJ));
        startGame(clients[fdA], clients[fdB], "RANKED");
        matched = true;
      }
    }
  }
}

// Gọi mỗi vòng lặp select (ít nhất 1 lần/giây)
void matchmakerTick()
{
  matchRanked();
}

void handleFindMatch(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 1)
  { // FIND_MATCH không có trường nào
    sendMsg(c, "400|FIND_MATCH|BAD_FORMAT");
    return;
  }
  cancelInvitesTo(c);
  queueOf(c).push_back(c.fd); // xếp vào cuối hàng
  c.state = ConnState::QUEUED;
  c.queuedAt = time(nullptr);             // ← THÊM: giờ hiện tại, tính bằng giây
  sendMsg(c, "202|FIND_MATCH|" + c.mode); // gửi 202 TRƯỚC...

  if (c.mode == "RANDOM")
    matchRandom(); // ...rồi mới thử ghép (có thể ra GAME_START ngay)
  else
    matchRanked();
}

void leaveQueue(Client &c)
{
  std::deque<int> &q = queueOf(c);
  q.erase(std::remove(q.begin(), q.end(), c.fd), q.end()); // xóa fd này khỏi hàng
  c.state = ConnState::IDLE;
}

void handleCancelMatch(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 1)
  {
    sendMsg(c, "400|CANCEL_MATCH|BAD_FORMAT");
    return;
  }
  leaveQueue(c);
  sendMsg(c, "200|CANCEL_MATCH");
}