#include "rooms.h"
#include "room.h"
#include "store.h"

const int INVITE_SECONDS = 20; // lời mời sống 20 giây

void handleCreateRoom(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 1)
  {
    sendMsg(c, "400|CREATE_ROOM|BAD_FORMAT");
    return;
  }
  cancelInvitesTo(c);
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
  r.invites[target] = time(nullptr) + INVITE_SECONDS; // mời lại = gia hạn thêm 20 giây
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

// c sắp rời IDLE: hủy mọi lời mời tới c, báo cho từng chủ phòng
void cancelInvitesTo(Client &c)
{
  for (auto &[id, r] : rooms)
  {
    if (r.invites.erase(c.user) == 0)
      continue; // phòng này không mời c → bỏ qua
    Client *host = findByUser(r.host);
    if (host)
      sendMsg(*host, "INVITE_DECLINED|" + c.user);
  }
}

void handleAcceptInvite(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 2)
  {
    sendMsg(c, "400|ACCEPT_INVITE|BAD_FORMAT");
    return;
  }
  auto it = rooms.find(f[1]);

  // 1. Đã được mời nhưng người khác vào trước (ACCEPT đến sau INVITE_CANCELLED) → phòng đã đủ
  if (it != rooms.end() && it->second.lateInvitees.count(c.user))
  {
    sendMsg(c, "410|ACCEPT_INVITE|ROOM_FULL");
    return;
  }
  // 2. Có phòng này và phòng có mời mình không?
  if (it == rooms.end() || it->second.invites.count(c.user) == 0)
  {
    sendMsg(c, "404|ACCEPT_INVITE|NO_SUCH_INVITE");
    return;
  }
  Room &r = it->second;

  // 3. Dọn lời mời
  r.invites.erase(c.user); // lời mời của chính mình
  cancelInvitesTo(c);      // ② lời mời từ phòng khác tới mình
  for (const auto &[name, expires] : r.invites)
  {
    Client *other = findByUser(name); // ① người khác được mời vào phòng này
    if (other)
      sendMsg(*other, "INVITE_CANCELLED|" + r.id);
    r.lateInvitees.insert(name); // nếu họ vẫn kịp gửi ACCEPT_INVITE → 410 ROOM_FULL
  }
  r.invites.clear();

  // 4. Trả lời rồi bắt đầu ván
  sendMsg(c, "200|ACCEPT_INVITE|" + r.id); // response TRƯỚC, event GAME_START SAU
  Client *host = findByUser(r.host);
  beginGame(r, *host, c); // hàm đã tách ở bước 5.1
}

void handleDeclineInvite(Client &c, const std::vector<std::string> &f)
{
  if (f.size() != 2)
  {
    sendMsg(c, "400|DECLINE_INVITE|BAD_FORMAT");
    return;
  }
  auto it = rooms.find(f[1]);
  if (it == rooms.end() || it->second.invites.count(c.user) == 0)
  {
    sendMsg(c, "404|DECLINE_INVITE|NO_SUCH_INVITE");
    return;
  }
  Room &r = it->second;
  r.invites.erase(c.user);
  sendMsg(c, "200|DECLINE_INVITE|" + r.id);

  Client *host = findByUser(r.host);
  if (host)
    sendMsg(*host, "INVITE_DECLINED|" + c.user);
}

// Gọi mỗi giây: xóa lời mời quá hạn
void roomTick()
{
  time_t now = time(nullptr);
  for (auto &[id, r] : rooms)
  {
    for (auto it = r.invites.begin(); it != r.invites.end();)
    {
      if (it->second > now)
      {
        ++it;
        continue;
      } // chưa hết hạn → sang cái tiếp

      std::string name = it->first; // chép tên ra TRƯỚC khi xóa
      it = r.invites.erase(it);     // xóa, nhận về phần tử kế tiếp

      Client *invitee = findByUser(name);
      if (invitee)
        sendMsg(*invitee, "INVITE_CANCELLED|" + id);
      Client *host = findByUser(r.host);
      if (host)
        sendMsg(*host, "INVITE_DECLINED|" + name);
    }
  }
}