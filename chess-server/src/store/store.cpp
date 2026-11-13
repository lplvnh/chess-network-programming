#include "store.h"
#include <fstream> // ifstream (đọc file), ofstream (ghi file)
#include <sstream> // stringstream (tách chuỗi)
#include <vector>
#include <cstdio> // rename

std::map<std::string, User> g_users;
static std::string g_path; // nhớ đường dẫn file để lúc save dùng lại

bool storeLoad(const std::string &path) // đọc users.txt vào g_users (gọi là load)
{
  g_path = path;
  std::ifstream in(path);
  if (!in)
    return false; // chưa có file (lần chạy đầu tiên) → không sao

  std::string line;
  while (std::getline(in, line))
  { // đọc từng dòng
    // tách dòng theo '|'
    std::vector<std::string> f;
    std::stringstream ss(line);
    std::string part;
    while (std::getline(ss, part, '|'))
      f.push_back(part);
    if (f.size() != 6)
      continue; // dòng hỏng → bỏ qua

    try
    {
      User u;
      u.name = f[0];
      u.pass = f[1];
      u.rating = std::stoi(f[2]); // stoi: string → int
      u.wins = std::stoi(f[3]);
      u.draws = std::stoi(f[4]);
      u.losses = std::stoi(f[5]);
      g_users[u.name] = u;
    }
    catch (...)
    {
      continue; // số bị hỏng → bỏ qua dòng này
    }
  }
  return true;
}

bool storeSave() // ghi toàn bộ g_users ra file (gọi là save)
{
  std::string tmp = g_path + ".tmp";
  {
    std::ofstream out(tmp); // mở file tạm để ghi (xóa nội dung cũ)
    if (!out)
      return false;
    for (const auto &[name, u] : g_users)
    {
      out << u.name << '|' << u.pass << '|' << u.rating << '|'
          << u.wins << '|' << u.draws << '|' << u.losses << '\n';
    }
    if (!out)
      return false; // lỗi trong lúc ghi (đầy đĩa...)
  } // ← ra khỏi { } thì out tự đóng file, dữ liệu được ghi xuống đĩa
  return std::rename(tmp.c_str(), g_path.c_str()) == 0; // đổi tên: nguyên tử
}