#include "dispatcher.h"
#include "logger.h"
#include "lobby.h"
#include "matchmaker.h"
#include "rooms.h"
#include "game.h"
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

static void handlePing(Client &c, const Fields &)
{ // bỏ tên "f" vì không dùng → hết cảnh báo
  sendMsg(c, "200|PING");
}

using S = ConnState; // đặt tên ngắn cho đỡ dài dòng

// Một vài tập trạng thái hay dùng
const std::set<ConnState> ANY = {S::CONNECTED, S::IDLE, S::QUEUED, S::ROOM_WAITING, S::IN_GAME};
const std::set<ConnState> NOT_LOGGED = {S::CONNECTED};
const std::set<ConnState> LOGGED_IN = {S::IDLE, S::QUEUED, S::ROOM_WAITING, S::IN_GAME};

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
    {"LIST_ONLINE", {handleListOnline, LOGGED_IN}},
    {"LEADERBOARD", {handleLeaderboard, LOGGED_IN}},
    {"FIND_MATCH", {handleFindMatch, {S::IDLE}}},
    {"CANCEL_MATCH", {handleCancelMatch, {S::QUEUED}}},
    {"CREATE_ROOM", {handleCreateRoom, {S::IDLE}}},
    {"INVITE", {handleInvite, {S::ROOM_WAITING}}},
    {"LEAVE_ROOM", {handleLeaveRoom, {S::ROOM_WAITING}}},
    {"ACCEPT_INVITE", {handleAcceptInvite, {S::IDLE}}},
    {"DECLINE_INVITE", {handleDeclineInvite, {S::IDLE}}},
    {"MOVE", {handleMove, {S::IN_GAME}}},
    {"RESIGN", {handleResign, {S::IN_GAME}}},
    {"OFFER_DRAW", {handleOfferDraw, {S::IN_GAME}}},
    {"ACCEPT_DRAW", {handleAcceptDraw, {S::IN_GAME}}},
    {"DECLINE_DRAW", {handleDeclineDraw, {S::IN_GAME}}},
};
// line: bản tin đã bỏ ký tự kết thúc. Log RECV nguyên văn đã được ghi trong main.cpp
// ngay khi framer tách được bản tin, trước khi gọi hàm này.
void dispatch(Client &c, const std::string &line)
{
  // (Dòng quá 512 byte đã bị Framer chặn trước khi tới đây: LINE_TOO_LONG.)
  // Dòng rỗng không có tên lệnh, nhưng server không bao giờ im lặng → báo lệnh không tồn tại
  if (line.empty())
  {
    sendMsg(c, "400|ERROR|UNKNOWN_COMMAND");
    return;
  }

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

  // 3. Cú pháp chung: field cần ít nhất 1 ký tự (ví dụ LOGIN|alice||RANKED → BAD_FORMAT)
  for (size_t i = 1; i < f.size(); i++)
    if (f[i].empty())
    {
      sendMsg(c, "400|" + cmd + "|BAD_FORMAT");
      return;
    }

  // 4. Hợp lệ → gọi handler (handler tự kiểm tra số trường, định dạng riêng)
  command.handler(c, f);
}