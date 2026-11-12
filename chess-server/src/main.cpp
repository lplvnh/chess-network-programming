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

  std::vector<int> clients; // danh sách fd của các client đang kết nối

  while (true)
  {
    // 1. Gom danh sách socket cần theo dõi
    fd_set readfds;
    FD_ZERO(&readfds);          // xóa sạch danh sách
    FD_SET(listenFd, &readfds); // theo dõi socket lễ tân
    int maxfd = listenFd;
    for (int fd : clients)
    {
      FD_SET(fd, &readfds);        // theo dõi từng client
      maxfd = std::max(maxfd, fd); // select cần biết fd lớn nhất
    }

    // 2. Chờ tới khi có socket nào đó sẵn sàng
    select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);

    // 3. Socket lễ tân sẵn sàng = có người mới kết nối
    if (FD_ISSET(listenFd, &readfds))
    {
      int fd = accept(listenFd, nullptr, nullptr);
      clients.push_back(fd);
      printf("Client connected, fd=%d\n", fd);
    }

    // 4. Client nào sẵn sàng thì đọc client đó
    std::vector<int> closed; // những client ngắt trong vòng này
    for (int fd : clients)
    {
      if (!FD_ISSET(fd, &readfds))
        continue; // không có dữ liệu → bỏ qua

      char buf[1024];
      ssize_t r = recv(fd, buf, sizeof(buf) - 1, 0);
      if (r <= 0)
      { // client ngắt kết nối
        printf("Client fd=%d disconnected\n", fd);
        close(fd);
        closed.push_back(fd);
        continue;
      }
      buf[r] = '\0';
      printf("fd=%d received %zd bytes: %s", fd, r, buf);
    }

    // 5. Xóa các client đã ngắt khỏi danh sách
    for (int fd : closed)
      clients.erase(std::find(clients.begin(), clients.end(), fd));
  }
}

// Code cũ bị kẹt vì accept() và recv() đều đứng chờ. Đang chờ client 1 thì không làm được việc gì khác.

// select() thay đổi cách chờ: thay vì chờ từng cái một, ta đưa cho nó cả danh sách socket và bảo “khi nào cái
// nào có dữ liệu thì báo tôi”. Nó chờ một lần cho tất cả, rồi cho biết socket nào đã sẵn sàng. Lúc đó gọi accept()
// hay recv() trên socket đó sẽ không bị chờ nữa, vì chắc chắn đã có dữ liệu.