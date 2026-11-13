#include "connection.h"
#include "logger.h"
#include "framer.h"
#include "dispatcher.h"
#include "store.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h> // ← MỚI: select, fd_set
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <vector>    // ← MỚI: danh sách client
#include <algorithm> // ← MỚI: std::max, std::find

int main()
{
  logOpen("server.log");
  storeLoad("users.txt");
  printf("Loaded %zu users\n", g_users.size());
  int listenFd = socket(AF_INET, SOCK_STREAM, 0);

  // MỚI: cho phép chạy lại server ngay sau khi tắt,
  // không bị lỗi "Address already in use"
  int yes = 1;
  setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(5500);
  bind(listenFd, (sockaddr *)&addr, sizeof(addr));
  listen(listenFd, 16);
  printf("Server listening on port 5500\n");

  while (true)
  {
    // 1. Gom danh sách socket cần theo dõi
    fd_set readfds;
    FD_ZERO(&readfds);          // xóa sạch danh sách
    FD_SET(listenFd, &readfds); // theo dõi socket lễ tân
    int maxfd = listenFd;

    for (auto &[fd, c] : clients)
    { // ← đổi: duyệt cặp (fd, buffer)
      FD_SET(fd, &readfds);
      maxfd = std::max(maxfd, fd);
    }

    // 2. Chờ tới khi có socket nào đó sẵn sàng
    select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);

    // 3. Socket lễ tân sẵn sàng = có người mới kết nối
    if (FD_ISSET(listenFd, &readfds))
    {
      sockaddr_in ca{}; // ← MỚI: chỗ chứa địa chỉ client
      socklen_t len = sizeof(ca);
      int fd = accept(listenFd, (sockaddr *)&ca, &len); // ← trước đây là nullptr, nullptr
      Client c;
      c.fd = fd;
      c.addr = std::string(inet_ntoa(ca.sin_addr)) + ":" + std::to_string(ntohs(ca.sin_port));
      clients[fd] = c;
      logMsg("CONN", clients[fd], "connected");
    }

    // 4. Client nào sẵn sàng thì đọc client đó
    std::vector<int> closed; // những client ngắt trong vòng này
    for (auto &[fd, c] : clients)
    { // ← buffer → c
      if (!FD_ISSET(fd, &readfds))
        continue;

      char buf[1024];
      ssize_t r = recv(fd, buf, sizeof(buf), 0);
      if (r <= 0)
      {
        logMsg("DISC", c, "disconnected"); // ← printf → logMsg
        close(fd);
        closed.push_back(fd);
        continue;
      }
      c.buffer.append(buf, r); // ← buffer → c.buffer (cả dưới nữa)

      bool tooLong;
      for (const std::string &line : extractLines(c.buffer, tooLong))
        dispatch(c, line); // tên mới của handleLine (xem 1.6e)
      if (tooLong)
        sendMsg(c, "400|ERROR|LINE_TOO_LONG");
    }

    // 5. Xóa các client đã ngắt khỏi danh sách
    for (int fd : closed)
      clients.erase(fd); // ← đổi: map xóa theo key, gọn hơn
  }
}