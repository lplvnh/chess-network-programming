// Logger: ghi mọi bản tin nhận/gửi ra terminal và file server.log
#pragma once
#include <string>

namespace Logger {
    bool open(const std::string& path);           // mở file log (ghi nối tiếp)
    // tag: CONN / RECV / SEND / DISC
    void log(const std::string& tag, int fd, const std::string& addr,
             const std::string& user, const std::string& text);
}
