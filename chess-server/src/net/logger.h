#pragma once
#include <string>

struct Client; // "có một struct tên Client", không cần include cả connection.h

void logOpen(const std::string &path);
void logMsg(const std::string &tag, const Client &c, const std::string &text);