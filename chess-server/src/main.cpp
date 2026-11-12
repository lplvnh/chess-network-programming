#include <arpa/inet.h>  // htons
#include <netinet/in.h> // sockaddr_in
#include <sys/socket.h> // socket, bind, listen, accept, recv
#include <unistd.h>     // close
#include <cstdio>       // printf

int main()
{
  // 1. Tạo socket TCP, nhận về một con số gọi là file descriptor (fd)
  int listenFd = socket(AF_INET, SOCK_STREAM, 0);

  // 2. Gắn socket vào cổng 5500
  sockaddr_in addr{};
  addr.sin_family = AF_INET;         // IPv4
  addr.sin_addr.s_addr = INADDR_ANY; // nhận kết nối từ mọi máy
  addr.sin_port = htons(5500);       // htons: đổi số cổng sang thứ tự byte mạng
  bind(listenFd, (sockaddr *)&addr, sizeof(addr));

  // 3. Bắt đầu nghe, tối đa 16 kết nối xếp hàng chờ
  listen(listenFd, 16);
  printf("Server listening on port 5500\n");

  // 4. Đứng chờ cho đến khi có 1 client kết nối
  int clientFd = accept(listenFd, nullptr, nullptr);
  printf("Client connected, fd=%d\n", clientFd);

  // 5. Nhận gì in nấy, cho đến khi client ngắt
  char buf[1024];
  while (true)
  {
    ssize_t r = recv(clientFd, buf, sizeof(buf) - 1, 0); // đứng chờ dữ liệu
    if (r <= 0)
      break;       // 0 = client đóng kết nối, <0 = lỗi
    buf[r] = '\0'; // thêm ký tự kết thúc chuỗi để printf được
    printf("Received %zd bytes: %s", r, buf);
  }

  printf("Client disconnected\n");
  close(clientFd);
  close(listenFd);
}