#include "matchmaker.h"
#include "room.h"
#include <deque>
#include <algorithm>

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

void handleFindMatch(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 1)
  { // FIND_MATCH không có trường nào
    sendMsg(c, "400|FIND_MATCH|BAD_FORMAT");
    return;
  }
  queueOf(c).push_back(c.fd); // xếp vào cuối hàng
  c.state = ConnState::QUEUED;
  sendMsg(c, "202|FIND_MATCH|" + c.mode); // gửi 202 TRƯỚC...

  if (c.mode == "RANDOM")
    matchRandom(); // ...rồi mới thử ghép (có thể ra GAME_START ngay)
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