// Chess server - vòng lặp chính dùng select()
#include "connection.h"
#include "dispatcher.h"
#include "framer.h"
#include "logger.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static int createListener(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); // chạy lại ngay không bị "Address in use"

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;      // nhận kết nối từ mọi máy (cần cho 2 PC)
    addr.sin_port = htons(port);
    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); exit(1); }
    if (listen(fd, 16) < 0) { perror("listen"); exit(1); }
    return fd;
}

static void acceptClient(int listenFd) {
    sockaddr_in ca{};
    socklen_t len = sizeof(ca);
    int fd = accept(listenFd, (sockaddr*)&ca, &len);
    if (fd < 0) { perror("accept"); return; }
    if (fd >= FD_SETSIZE) { close(fd); return; }   // select() chỉ theo dõi được < FD_SETSIZE

    Client c;
    c.fd = fd;
    c.addr = std::string(inet_ntoa(ca.sin_addr)) + ":" + std::to_string(ntohs(ca.sin_port));
    g_clients[fd] = c;
    Logger::log("CONN", fd, c.addr, "", "connected");
}

// Đọc dữ liệu từ 1 client, tách dòng, xử lý từng dòng
static void readClient(Client& c) {
    char buf[1024];
    ssize_t r = recv(c.fd, buf, sizeof(buf), 0);
    if (r <= 0) {
        c.closing = true;
        return;
    }
    c.buffer.append(buf, (size_t)r);

    bool tooLong;
    std::vector<std::string> lines = extractLines(c.buffer, tooLong);
    for (const std::string& line : lines) {
        if (c.closing) break;
        Logger::log("RECV", c.fd, c.addr, c.user, line);
        if (line.empty()) continue;
        dispatch(c, line);
    }
    if (tooLong) sendMsg(c, "400|ERROR|LINE_TOO_LONG");
}

// Dọn các client bị đánh dấu closing (bước sau: hủy hàng đợi, xử thua ván...)
static void cleanupClosed() {
    for (auto it = g_clients.begin(); it != g_clients.end();) {
        Client& c = it->second;
        if (c.closing) {
            Logger::log("DISC", c.fd, c.addr, c.user, "disconnected");
            close(c.fd);
            it = g_clients.erase(it);
        } else {
            ++it;
        }
    }
}

int main(int argc, char* argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : 5500;
    if (!Logger::open("server.log")) { perror("server.log"); return 1; }

    int listenFd = createListener(port);
    printf("Chess server listening on port %d\n", port);

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenFd, &readfds);
        int maxfd = listenFd;
        for (auto& [fd, c] : g_clients) {
            FD_SET(fd, &readfds);
            if (fd > maxfd) maxfd = fd;
        }

        timeval tv{1, 0};   // 1 giây: nhịp cho xử lý định kỳ
        int n = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
        if (n < 0) { perror("select"); continue; }

        if (FD_ISSET(listenFd, &readfds)) acceptClient(listenFd);

        for (auto& [fd, c] : g_clients) {
            if (FD_ISSET(fd, &readfds)) readClient(c);
        }

        // (bước sau) xử lý định kỳ: ghép RANKED, hết hạn lời mời, heartbeat

        cleanupClosed();
    }
}
