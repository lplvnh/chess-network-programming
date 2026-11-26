#include "connection.h"
#include "logger.h"
#include <sys/socket.h>

std::map<int, Client> clients; // biến thật được tạo ở đây

// static = hàm "riêng tư", chỉ dùng trong file này
static bool sendAll(int fd, const std::string &data)
{
  size_t sent = 0;
  while (sent < data.size())
  {
    ssize_t r = send(fd, data.c_str() + sent, data.size() - sent, MSG_NOSIGNAL);
    if (r <= 0)
      return false; // lỗi (client đã ngắt...)
    sent += r;
  }
  return true;
}

// Ghép '\n' TRƯỚC, log đúng chuỗi byte sắp gửi, rồi mới gửi
void sendMsg(Client &c, const std::string &msg)
{
  std::string data = msg + "\n";
  logMsg("SEND", c, data);
  sendAll(c.fd, data);
}

// Response nhiều dòng (200|LIST_ONLINE|n, 200|LEADERBOARD|k...): theo cú pháp
// response = response-line *data-line, cả khối là MỘT bản tin, nên được ghép đủ,
// log MỘT lần đúng chuỗi byte sắp gửi, rồi gửi trong một lần sendAll
void sendBlock(Client &c, const std::vector<std::string> &lines)
{
  std::string data;
  for (const std::string &line : lines)
    data += line + "\n";
  logMsg("SEND", c, data);
  sendAll(c.fd, data);
}

// Trả về con trỏ tới client đang đăng nhập bằng 'name', hoặc nullptr nếu không ai
Client *findByUser(const std::string &name)
{
  for (auto &[fd, c] : clients)
    if (c.user == name)
      return &c; // &c = địa chỉ của c
  return nullptr;
}