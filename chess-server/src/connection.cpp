#include "connection.h"
#include "logger.h"
#include <sys/socket.h>
#include <sstream>

std::map<int, Client> g_clients;

// Gửi hết chuỗi, vì send() có thể chỉ gửi được một phần (vòng lặp nLeft trong bài giảng)
static bool sendAll(int fd, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t r = send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (r <= 0) return false;
        sent += (size_t)r;
    }
    return true;
}

void sendBlock(Client& c, const std::string& block) {
    if (c.closing) return;
    // log từng dòng cho dễ đọc
    std::istringstream ss(block);
    std::string line;
    while (std::getline(ss, line)) Logger::log("SEND", c.fd, c.addr, c.user, line);
    if (!sendAll(c.fd, block)) c.closing = true;
}

void sendMsg(Client& c, const std::string& msg) {
    sendBlock(c, msg + "\n");
}
