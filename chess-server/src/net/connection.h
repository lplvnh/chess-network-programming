#pragma once // chống include 2 lần
#include <string>
#include <map>
#include <vector>

// 5 trạng thái của một kết nối (state machine, mục 5.6 trong design)
enum class ConnState
{
  CONNECTED,
  IDLE,
  QUEUED,
  ROOM_WAITING,
  IN_GAME
};

struct Client
{
  int fd;
  std::string addr;
  std::string buffer;
  std::string user;                       // ← MỚI: rỗng = chưa đăng nhập
  std::string mode;                       // ← MỚI: "RANDOM" hoặc "RANKED"
  ConnState state = ConnState::CONNECTED; // ← MỚI
};

extern std::map<int, Client> clients; // extern = biến thật nằm ở .cpp

void sendMsg(Client &c, const std::string &msg); // chỉ khai báo tên hàm
void sendBlock(Client &c, const std::vector<std::string> &lines);

Client *findByUser(const std::string &name); // tìm client đang đăng nhập bằng tên này