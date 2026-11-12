#pragma once // chống include 2 lần
#include <string>
#include <map>

struct Client
{
  int fd;
  std::string addr;
  std::string buffer;
};

extern std::map<int, Client> clients; // extern = biến thật nằm ở .cpp

void sendMsg(Client &c, const std::string &msg); // chỉ khai báo tên hàm