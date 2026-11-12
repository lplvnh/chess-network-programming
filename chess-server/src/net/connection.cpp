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