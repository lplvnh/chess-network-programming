#include "auth.h"
#include "store.h"
#include <cctype> // isalnum
#include "password.h"

// Tên hợp lệ: 3–20 ký tự, chỉ gồm chữ, số, dấu _
static bool validUser(const std::string &u)
{
  if (u.size() < 3 || u.size() > 20)
    return false;
  for (char ch : u)
    if (!isalnum((unsigned char)ch) && ch != '_')
      return false;
  return true;
}

// Mật khẩu hợp lệ: 4–32 ký tự in được, không có dấu cách
static bool validPass(const std::string &p)
{
  if (p.size() < 4 || p.size() > 32)
    return false;
  for (char ch : p)
    if (ch <= ' ' || ch > '~')
      return false;
  return true;
}

// f = các trường đã cắt theo '|', ví dụ {"REGISTER", "alice", "secret12"}
void handleRegister(Client &c, const std::vector<std::string> &f)
{
  // 1. Kiểm tra định dạng
  if (f.size() != 3 || !validUser(f[1]) || !validPass(f[2]))
  {
    sendMsg(c, "400|REGISTER|BAD_FORMAT");
    return;
  }
  const std::string &name = f[1];

  // 2. Tên đã có chưa?
  if (g_users.count(name))
  { // count() trả 1 nếu có, 0 nếu không
    sendMsg(c, "409|REGISTER|USERNAME_TAKEN");
    return;
  }

  // 3. Lưu user mới
  User u;
  u.name = name;
  u.pass = hashPassword(f[2]);
  g_users[name] = u;

  if (!storeSave())
  {                      // ghi file thất bại
    g_users.erase(name); // hủy luôn trong RAM, để RAM và file khớp nhau
    sendMsg(c, "500|REGISTER|SERVER_ERROR");
    return;
  }
  sendMsg(c, "200|REGISTER|" + name);
}

void handleLogin(Client &c, const std::vector<std::string> &f)
{
  // 1. Định dạng: LOGIN|user|pass|mode
  if (f.size() != 4 || (f[3] != "RANDOM" && f[3] != "RANKED"))
  {
    sendMsg(c, "400|LOGIN|BAD_FORMAT");
    return;
  }
  const std::string &name = f[1];
  const std::string &pass = f[2];
  const std::string &mode = f[3];

  // 2. Tên và mật khẩu (sai cái nào cũng trả cùng một lỗi)
  auto it = g_users.find(name);
  if (it == g_users.end() || !checkPassword(pass, it->second.pass))
  {
    sendMsg(c, "403|LOGIN|INVALID_CREDENTIALS");
    return;
  }

  // 3. Đang online ở kết nối khác?
  if (findByUser(name) != nullptr)
  {
    sendMsg(c, "409|LOGIN|ALREADY_ONLINE");
    return;
  }

  // 4. Gắn user vào kết nối
  c.user = name;
  c.mode = mode;
  c.state = ConnState::IDLE;
  const User &u = it->second;
  sendMsg(c, "200|LOGIN|" + u.name + "|" + std::to_string(u.rating) + "|" + mode);
}

void handleLogout(Client &c, const std::vector<std::string> &)
{
  sendMsg(c, "200|LOGOUT"); // gửi TRƯỚC khi xóa tên, để log còn ghi user=alice
  c.user.clear();
  c.mode.clear();
  c.state = ConnState::CONNECTED;
}