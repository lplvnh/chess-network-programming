#include "cleanup.h"
#include "matchmaker.h"
#include "rooms.h"
#include "game.h"

void cleanupClient(Client &c)
{
  switch (c.state)
  {
  case ConnState::IDLE:
    cancelInvitesTo(c);
    break; // lời mời tới mình
  case ConnState::QUEUED:
    leaveQueue(c);
    break; // hàng đợi
  case ConnState::ROOM_WAITING:
    closeRoom(c);
    break; // phòng của mình
  case ConnState::IN_GAME:
    forfeitOnDisconnect(c);
    break; // ← THÊM
  default:
    break; // CONNECTED: không có gì; IN_GAME: làm ở bước kết thúc ván
  }
}