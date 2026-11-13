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

void sendMsg(Client &c, const std::string &msg)
{
  logMsg("SEND", c, msg);
  sendAll(c.fd, msg + "\n");
}

// Gửi nhiều dòng trong MỘT lần sendAll. Log từng dòng riêng cho dễ đọc
void sendBlock(Client &c, const std::vector<std::string> &lines)
{
  std::string data;
  for (const std::string &line : lines)
  {
    logMsg("SEND", c, line);
    data += line + "\n"; // ghép tất cả thành 1 chuỗi
  }
  sendAll(c.fd, data); // gửi một lần
}

// Trả về con trỏ tới client đang đăng nhập bằng 'name', hoặc nullptr nếu không ai
Client *findByUser(const std::string &name)
{
  for (auto &[fd, c] : clients)
    if (c.user == name)
      return &c; // &c = địa chỉ của c
  return nullptr;
}