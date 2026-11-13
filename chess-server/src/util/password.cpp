#include "password.h"
#include <crypt.h> // crypt_r
#include <fstream> // đọc /dev/urandom

// Tạo salt 16 ký tự ngẫu nhiên
static std::string makeSalt()
{
  // crypt chỉ chấp nhận 64 ký tự này trong salt
  const char *chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
  unsigned char bytes[16];
  std::ifstream urandom("/dev/urandom", std::ios::binary); // nguồn ngẫu nhiên của Linux
  urandom.read((char *)bytes, sizeof(bytes));

  std::string salt;
  for (unsigned char b : bytes)
    salt += chars[b % 64]; // mỗi byte → 1 ký tự hợp lệ
  return salt;
}

// "secret12" → "$5$<salt>$<hash>"
std::string hashPassword(const std::string &pass)
{
  std::string setting = "$5$" + makeSalt() + "$"; // $5$ = SHA-256
  crypt_data data{};
  const char *result = crypt_r(pass.c_str(), setting.c_str(), &data);
  return result ? result : "";
}

// Hash lại mật khẩu vừa nhập với CÙNG salt đã lưu, rồi so sánh
bool checkPassword(const std::string &pass, const std::string &stored)
{
  crypt_data data{};
  const char *result = crypt_r(pass.c_str(), stored.c_str(), &data);
  return result && stored == result;
}