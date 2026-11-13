#include "dispatcher.h"
#include "logger.h"
#include <vector>
#include <functional>
#include "auth.h"
#include <set> // std::set: tập hợp, không trùng phần tử

using Fields = std::vector<std::string>;
using Handler = std::function<void(Client &c, const Fields &f)>;

// ---------- Cắt trường ----------
// "LOGIN|alice|123" → {"LOGIN", "alice", "123"}
static Fields splitFields(const std::string &line)
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

// Tạo bản sao của dòng để GHI LOG, với mật khẩu đã che bằng ***
static std::string maskForLog(const std::string &line)
{
  Fields f = splitFields(line);
  if ((f[0] == "LOGIN" || f[0] == "REGISTER") && f.size() >= 3)
  {
    f[2] = "***"; // trường 2 = mật khẩu
    std::string out = f[0];
    for (size_t i = 1; i < f.size(); i++)
      out += "|" + f[i]; // nối lại bằng '|'
    return out;
  }
  return line; // lệnh khác → giữ nguyên
}

static void handlePing(Client &c, const Fields &)
{ // bỏ tên "f" vì không dùng → hết cảnh báo
  sendMsg(c, "200|PING");
}

using S = ConnState; // đặt tên ngắn cho đỡ dài dòng

// Một vài tập trạng thái hay dùng
const std::set<ConnState> ANY = {S::CONNECTED, S::IDLE, S::QUEUED, S::ROOM_WAITING, S::IN_GAME};
const std::set<ConnState> NOT_LOGGED = {S::CONNECTED};

// Một lệnh = hàm xử lý + những trạng thái được phép
struct Command
{
  Handler handler;
  std::set<ConnState> allowed;
};

static const std::map<std::string, Command> COMMANDS = {
    {"PING", {handlePing, ANY}},
    {"REGISTER", {handleRegister, NOT_LOGGED}},
    {"LOGIN", {handleLogin, NOT_LOGGED}},
    {"LOGOUT", {handleLogout, {S::IDLE, S::QUEUED, S::ROOM_WAITING}}},
};
void dispatch(Client &c, const std::string &line)
{
  logMsg("RECV", c, maskForLog(line));
  if (line.empty())
    return;

  Fields f = splitFields(line);
  const std::string &cmd = f[0];

  // 1. Lệnh có tồn tại?
  auto it = COMMANDS.find(cmd);
  if (it == COMMANDS.end())
  {
    sendMsg(c, "400|ERROR|UNKNOWN_COMMAND");
    return;
  }
  const Command &command = it->second;

  // 2. Trạng thái hiện tại có nằm trong danh sách cho phép?
  if (command.allowed.count(c.state) == 0)
  {
    if (c.state == ConnState::CONNECTED)
      sendMsg(c, "401|" + cmd + "|NOT_LOGGED_IN");
    else
      sendMsg(c, "405|" + cmd + "|WRONG_STATE");
    return;
  }

  // 3 + 4. Hợp lệ → gọi handler (handler tự kiểm tra định dạng)
  command.handler(c, f);
}