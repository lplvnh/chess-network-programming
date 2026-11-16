#include "rooms.h"
#include "room.h"
#include "store.h"

const int INVITE_SECONDS = 60; // lời mời sống 60 giây

void handleCreateRoom(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 1)
  {
    sendMsg(c, "400|CREATE_ROOM|BAD_FORMAT");
    return;
  }
  Room r;
  r.id = newRoomId();
  r.source = "ROOM";
  r.state = "WAITING";
  r.host = c.user;
  rooms[r.id] = r;

  c.roomId = r.id;
  c.state = ConnState::ROOM_WAITING;
  sendMsg(c, "201|CREATE_ROOM|" + r.id);
}

void handleInvite(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 2)
  {
    sendMsg(c, "400|INVITE|BAD_FORMAT");
    return;
  }
  const std::string &target = f[1];
  if (target == c.user)
  {
    sendMsg(c, "400|INVITE|SELF_INVITE");
    return;
  }
  Client *t = findByUser(target); // hàm từ bước 2.3
  if (t == nullptr)
  {
    sendMsg(c, "404|INVITE|USER_NOT_ONLINE");
    return;
  }
  if (t->state != ConnState::IDLE)
  {
    sendMsg(c, "409|INVITE|USER_BUSY");
    return;
  }

  Room &r = rooms[c.roomId];
  r.invites[target] = time(nullptr) + INVITE_SECONDS; // mời lại = gia hạn thêm 60 giây
  sendMsg(c, "200|INVITE|" + target);
  sendMsg(*t, "INVITED|" + c.user + "|" +
                  std::to_string(g_users[c.user].rating) + "|" + r.id);
}

void handleLeaveRoom(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 1)
  {
    sendMsg(c, "400|LEAVE_ROOM|BAD_FORMAT");
    return;
  }
  closeRoom(c);
  sendMsg(c, "200|LEAVE_ROOM");
}

// Hủy phòng đang chờ: báo cho mọi người đang được mời, rồi xóa phòng
void closeRoom(Client &host)
{
  Room &r = rooms[host.roomId];
  for (const auto &[name, expires] : r.invites)
  {
    Client *other = findByUser(name);
    if (other)
      sendMsg(*other, "INVITE_CANCELLED|" + r.id); // có thể họ đã offline
  }
  rooms.erase(r.id);
  host.roomId.clear();
  host.state = ConnState::IDLE;
}