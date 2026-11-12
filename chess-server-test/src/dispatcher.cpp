#include "dispatcher.h"
#include "framer.h"
#include <map>
#include <functional>
#include <vector>

using Fields = std::vector<std::string>;
using Handler = std::function<void(Client&, const Fields&)>;

// ---- Các hàm xử lý (bước sau sẽ thêm LOGIN, MOVE...) ----
static void handlePing(Client& c, const Fields&) {
    sendMsg(c, "200|PING");
}

// Bảng lệnh: tên lệnh -> hàm xử lý
static const std::map<std::string, Handler> HANDLERS = {
    {"PING", handlePing},
};

void dispatch(Client& c, const std::string& line) {
    Fields f = splitFields(line);
    auto it = HANDLERS.find(f[0]);
    if (it == HANDLERS.end()) {
        sendMsg(c, "400|ERROR|UNKNOWN_COMMAND");
        return;
    }
    // (bước sau) kiểm tra ma trận trạng thái: 401 NOT_LOGGED_IN / 405 WRONG_STATE
    it->second(c, f);
}
