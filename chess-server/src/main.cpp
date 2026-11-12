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
#include <sys/time.h> // ← MỚI: gettimeofday (lấy giờ có mili giây)
#include <ctime>      // ← MỚI: strftime (định dạng ngày giờ)

const size_t MAX_LINE = 512; // một dòng dài tối đa 512 byte (theo design)

struct Client
{
  int fd;
  std::string addr;   // "127.0.0.1:51144", để ghi log
  std::string buffer; // bộ đệm nhận (từ bước 1.3)
  // sau này thêm: std::string user; trạng thái; ...
};

std::map<int, Client> clients; // fd → Client. Đưa ra ngoài main để các hàm khác dùng được

// ---------- Logger ----------
FILE *logFile = nullptr; // file server.log, mở ở đầu main()

// Trả về giờ hiện tại dạng "2026-11-12 23:28:17.779"
std::string now()
{
  timeval tv;
  gettimeofday(&tv, nullptr); // lấy giờ (giây + micro giây)
  tm t;
  localtime_r(&tv.tv_sec, &t); // đổi sang giờ địa phương
  char buf[64];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
  char out[80];
  snprintf(out, sizeof(out), "%s.%03ld", buf, (long)(tv.tv_usec / 1000)); // thêm mili giây
  return out;
}

// tag: CONN / RECV / SEND / DISC
void logMsg(const std::string &tag, const Client &c, const std::string &text)
{
  std::string line = now() + " " + tag + " fd=" + std::to_string(c.fd) +
                     " " + c.addr + " : " + text;
  printf("%s\n", line.c_str()); // ra terminal
  fflush(stdout);
  if (logFile)
  {
    fprintf(logFile, "%s\n", line.c_str()); // ra file
    fflush(logFile);                        // ghi xuống đĩa ngay, server chết đột ngột vẫn còn log
  }
}

// ---------- Gửi ----------
// send() có thể chỉ gửi được một phần, nên phải lặp đến khi hết
bool sendAll(int fd, const std::string &data)
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

// CHỖ GỬI DUY NHẤT của cả server: ghi log rồi gửi
void sendMsg(Client &c, const std::string &msg)
{
  logMsg("SEND", c, msg);
  sendAll(c.fd, msg + "\n");
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
using Fields = std::vector<std::string>;                         // đặt tên ngắn cho dễ đọc
using Handler = std::function<void(Client &c, const Fields &f)>; // ← đổi int fd → Client& c

void handlePing(Client &c, const Fields &f)
{                         // ← đổi
  sendMsg(c, "200|PING"); // ← sendLine(fd,...) → sendMsg(c,...)
}

// ---------- Bảng lệnh ----------
const std::map<std::string, Handler> HANDLERS = {
    {"PING", handlePing},
    // sau này: {"REGISTER", handleRegister}, {"LOGIN", handleLogin}, ...
};

// ---------- Dispatcher ----------
// Được gọi mỗi khi tách được 1 dòng hoàn chỉnh (từ bước 1.3)
void handleLine(Client &c, const std::string &line)
{                          // ← đổi
  logMsg("RECV", c, line); // ← printf → logMsg
  if (line.empty())
    return;

  Fields f = splitFields(line);
  auto it = HANDLERS.find(f[0]);
  if (it == HANDLERS.end())
  {
    sendMsg(c, "400|ERROR|UNKNOWN_COMMAND"); // ← đổi
    return;
  }
  it->second(c, f); // ← đổi
}

int main()
{
  logFile = fopen("server.log", "a"); // "a" = append, ghi nối vào cuối file
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

      size_t pos;
      while ((pos = c.buffer.find('\n')) != std::string::npos)
      {
        std::string line = c.buffer.substr(0, pos);
        c.buffer.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r')
          line.pop_back();
        handleLine(c, line); // ← truyền c thay vì fd
      }

      if (c.buffer.size() > MAX_LINE)
      {
        c.buffer.clear();
        sendMsg(c, "400|ERROR|LINE_TOO_LONG"); // ← giờ trả lỗi cho client, đúng design
      }
    }

    // 5. Xóa các client đã ngắt khỏi danh sách
    for (int fd : closed)
      clients.erase(fd); // ← đổi: map xóa theo key, gọn hơn
  }
}

// Bước 1.5: Struct Client + gửi an toàn + Logger

// Mỗi client chỉ có fd và bộ đệm. Sau này cần thêm địa chỉ IP, tên user, trạng thái... -->	Gom hết vào struct Client
// send() có thể chỉ gửi được một phần dữ liệu (vòng lặp nLeft trong bài giảng) --> Hàm sendAll lặp cho đến khi gửi hết
// Log chỉ in ra màn hình, không có giờ, không có IP. Đề bài yêu cầu log mọi bản tin (1 điểm) --> Hàm logMsg: có giờ đến mili giây, có IP:port, ghi cả ra file server.log