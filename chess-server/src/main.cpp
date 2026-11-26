#include "connection.h"
#include "logger.h"
#include "framer.h"
#include "dispatcher.h"
#include "store.h"
#include "matchmaker.h"
#include "rooms.h"
#include "cleanup.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h> // ← MỚI: select, fd_set
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <vector>    // ← MỚI: danh sách client
#include <algorithm> // ← MỚI: std::max, std::find

#include <ctime> // ← THÊM ở phần include

const int HEARTBEAT_TIMEOUT = 45;

// Ngắt client c: ghi log, dọn dẹp theo trạng thái, đóng socket, đánh dấu để xóa
static void dropClient(Client &c, const std::string &reason, std::vector<int> &closed)
{
  logEvent("DISC", c, "reason=" + reason);
  cleanupClient(c);
  c.user.clear(); // để findByUser không tìm thấy client sắp xóa này nữa
  close(c.fd);
  closed.push_back(c.fd);
}

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
    timeval tv{1, 0};                                   // 1 giây, 0 micro giây
    select(maxfd + 1, &readfds, nullptr, nullptr, &tv); // nullptr → &tv

    // 3. Socket lễ tân sẵn sàng = có người mới kết nối
    if (FD_ISSET(listenFd, &readfds))
    {
      sockaddr_in ca{}; // ← MỚI: chỗ chứa địa chỉ client
      socklen_t len = sizeof(ca);
      int fd = accept(listenFd, (sockaddr *)&ca, &len); // ← trước đây là nullptr, nullptr
      Client c;
      c.fd = fd;
      c.lastSeen = time(nullptr);
      c.addr = std::string(inet_ntoa(ca.sin_addr)) + ":" + std::to_string(ntohs(ca.sin_port));
      clients[fd] = c;
      logEvent("CONN", clients[fd], "connected");
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
        dropClient(c, "recv_0", closed);
        continue;
      }
      c.lastSeen = time(nullptr); // ← có dữ liệu = còn sống
      c.buffer.append(buf, r);

      for (const Frame &fr : extractFrames(c.buffer, c.skipping))
      {
        if (fr.kind == FrameKind::MESSAGE)
        {
          logMsg("RECV", c, fr.raw);           // in NGAY khi nhận đủ một bản tin, nguyên văn
          dispatch(c, stripTerminator(fr.raw)); // rồi mới xử lý
        }
        else
        {
          logMsg("DROP", c, fr.raw); // byte bị bỏ vì dòng quá 512 byte (vẫn in đủ)
          if (fr.kind == FrameKind::TOO_LONG)
            sendMsg(c, "400|ERROR|LINE_TOO_LONG");
        }
      }
    }

    // Heartbeat: ai đã đăng nhập mà im lặng quá lâu thì coi như ngắt
    time_t now = time(nullptr);
    for (auto &[fd, c] : clients)
    {
      bool alreadyClosed = std::find(closed.begin(), closed.end(), fd) != closed.end();
      if (!alreadyClosed && !c.user.empty() && now - c.lastSeen > HEARTBEAT_TIMEOUT)
        dropClient(c, "timeout", closed);
    }

    matchmakerTick();
    roomTick();
    // 5. Xóa các client đã ngắt khỏi danh sách
    for (int fd : closed)
      clients.erase(fd); // ← đổi: map xóa theo key, gọn hơn
  }
}