// Connection Manager: thông tin từng client + hàm gửi duy nhất
#pragma once
#include <string>
#include <map>

enum class ConnState { CONNECTED, IDLE, QUEUED, ROOM_WAITING, IN_GAME };

struct Client {
    int fd;
    std::string addr;            // "IP:port"
    std::string buffer;          // bộ đệm nhận (dữ liệu chưa đủ 1 dòng)
    std::string user;            // rỗng = chưa đăng nhập
    ConnState state = ConnState::CONNECTED;
    bool closing = false;        // đánh dấu để dọn sau vòng lặp select
};

extern std::map<int, Client> g_clients;   // fd -> Client

// Gửi 1 bản tin (tự thêm '\n'), ghi log trước khi gửi, lặp send() đến khi hết
void sendMsg(Client& c, const std::string& msg);
// Gửi nguyên một khối nhiều dòng (đã có '\n' ở mỗi dòng), dùng cho LIST_ONLINE...
void sendBlock(Client& c, const std::string& block);
