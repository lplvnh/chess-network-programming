#pragma once
#include "connection.h"
#include <string>

void dispatch(Client &c, const std::string &line); // đổi tên từ handleLine