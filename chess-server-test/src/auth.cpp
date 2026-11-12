#include "auth.h"
#include "store.h"

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
      return false; // ' ' = 32, '~' = 126 trong bảng ASCII
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
  u.pass = f[2];
  g_users[name] = u;
  sendMsg(c, "200|REGISTER|" + name);
}