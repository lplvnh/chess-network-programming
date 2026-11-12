#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h> // ← MỚI: select, fd_set
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <vector>     // ← MỚI: danh sách client
#include <algorithm>  // ← MỚI: std::max, std::find
#include <functional> // ← MỚI: std::function, dùng để lưu hàm vào bảng
#include <map>        // ← MỚI
#include <string>     // ← MỚI

const size_t MAX_LINE = 512; // một dòng dài tối đa 512 byte (theo design)

// ---------- Gửi ----------
// Gửi 1 dòng về client, tự thêm '\n' ở cuối
void sendLine(int fd, const std::string &msg)
{
  std::string data = msg + "\n";
  send(fd, data.c_str(), data.size(), 0);
  printf("fd=%d SEND: %s\n", fd, msg.c_str());
}

// ---------- Cắt trường ----------
// "LOGIN|alice|123" → {"LOGIN", "alice", "123"}
std::vector<std::string> splitFields(const std::string &line)
{
  std::vector<std::string> fields;
  size_t start = 0, pos;
  while ((pos = line.find('|', start)) != std::string::npos)
  {
    fields.push_back(line.substr(start, pos - start)); // lấy đoạn trước '|'
    start = pos + 1;                                   // nhảy qua '|'
  }
  fields.push_back(line.substr(start)); // đoạn cuối cùng
  return fields;
}

// ---------- Các hàm xử lý lệnh ----------
using Fields = std::vector<std::string>;                      // đặt tên ngắn cho dễ đọc
using Handler = std::function<void(int fd, const Fields &f)>; // "kiểu" của một hàm xử lý

void handlePing(int fd, const Fields &f)
{
  sendLine(fd, "200|PING");
}

// ---------- Bảng lệnh ----------
const std::map<std::string, Handler> HANDLERS = {
    {"PING", handlePing},
    // sau này: {"REGISTER", handleRegister}, {"LOGIN", handleLogin}, ...
};

// ---------- Dispatcher ----------
// Được gọi mỗi khi tách được 1 dòng hoàn chỉnh (từ bước 1.3)
void handleLine(int fd, const std::string &line)
{
  printf("fd=%d RECV: %s\n", fd, line.c_str());
  if (line.empty())
    return; // dòng rỗng (lỡ bấm Enter) thì bỏ qua

  Fields f = splitFields(line);
  auto it = HANDLERS.find(f[0]); // f[0] là tên lệnh
  if (it == HANDLERS.end())
  { // không có trong bảng
    sendLine(fd, "400|ERROR|UNKNOWN_COMMAND");
    return;
  }
  it->second(fd, f); // it->second chính là hàm xử lý, gọi nó
}

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

  std::map<int, std::string> clients; // fd → bộ đệm của client đó

  while (true)
  {
    // 1. Gom danh sách socket cần theo dõi
    fd_set readfds;
    FD_ZERO(&readfds);          // xóa sạch danh sách
    FD_SET(listenFd, &readfds); // theo dõi socket lễ tân
    int maxfd = listenFd;

    for (auto &[fd, buffer] : clients)
    { // ← đổi: duyệt cặp (fd, buffer)
      FD_SET(fd, &readfds);
      maxfd = std::max(maxfd, fd);
    }

    // 2. Chờ tới khi có socket nào đó sẵn sàng
    select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);

    // 3. Socket lễ tân sẵn sàng = có người mới kết nối
    if (FD_ISSET(listenFd, &readfds))
    {
      int fd = accept(listenFd, nullptr, nullptr);
      // clients.push_back(fd);
      clients[fd] = "";
      printf("Client connected, fd=%d\n", fd);
    }

    // 4. Client nào sẵn sàng thì đọc client đó
    std::vector<int> closed; // những client ngắt trong vòng này
    for (auto &[fd, buffer] : clients)
    {
      if (!FD_ISSET(fd, &readfds))
        continue;

      char buf[1024];
      ssize_t r = recv(fd, buf, sizeof(buf), 0);
      if (r <= 0)
      {
        printf("Client fd=%d disconnected\n", fd);
        close(fd);
        closed.push_back(fd);
        continue;
      }
      printf("fd=%d recv %zd bytes\n", fd, r); // in ra để thấy mỗi lần recv nhận bao nhiêu

      // (a) nối dữ liệu mới vào cuối bộ đệm
      buffer.append(buf, r);

      // (b) còn '\n' thì cắt ra từng dòng
      size_t pos;
      while ((pos = buffer.find('\n')) != std::string::npos)
      {
        std::string line = buffer.substr(0, pos); // phần trước '\n'
        buffer.erase(0, pos + 1);                 // xóa dòng đó (kèm '\n') khỏi bộ đệm
        if (!line.empty() && line.back() == '\r') // gõ từ telnet/Windows có "\r\n"
          line.pop_back();                        //   → bỏ '\r' ở cuối
        handleLine(fd, line);
      }

      // (c) chống client gửi mãi không có '\n' làm đầy bộ nhớ
      if (buffer.size() > MAX_LINE)
      {
        printf("fd=%d line too long, dropped\n", fd);
        buffer.clear();
      }
    }

    // 5. Xóa các client đã ngắt khỏi danh sách
    for (int fd : closed)
      clients.erase(fd); // ← đổi: map xóa theo key, gọn hơn
  }
}

// Bước 1.4: Dispatcher, server bắt đầu trả lời

// Tới giờ server mới chỉ in ra các dòng nhận được. Giờ nó phải hiểu và trả lời theo từng lệnh

// "PING"         → cắt theo '|' → ["PING"]        → tìm "PING" trong bảng → handlePing  → gửi "200|PING"
// "HELLO"        → cắt theo '|' → ["HELLO"]       → không có trong bảng                → gửi "400|ERROR|UNKNOWN_COMMAND"