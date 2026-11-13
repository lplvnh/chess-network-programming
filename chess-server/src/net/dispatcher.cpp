#include "dispatcher.h"
#include "logger.h"
#include <vector>
#include <functional>
#include "auth.h"

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

static const std::map<std::string, Handler> HANDLERS = {
    {"PING", handlePing},
    {"REGISTER", handleRegister},
};

void dispatch(Client &c, const std::string &line)
{
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