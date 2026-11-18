#include "chess.h"
#include <cstdio>

// In mọi nước đi hợp lệ của một thế cờ
static void listMoves(const char *title, const std::string &fen)
{
  Position p;
  parseFen(fen, p);
  std::vector<Move> moves = legalMoves(p);
  printf("%s: %zu moves\n  ", title, moves.size());
  for (const Move &m : moves)
    printf("%s ", moveToUci(m).c_str());
  printf("\n\n");
}

int main()
{
  // 1. Thế cờ ban đầu: phải đúng 20 nước (16 nước tốt + 4 nước mã)
  listMoves("Start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

  // 2. Hậu đứng giữa bàn trống: 27 nước
  listMoves("Lone queen d4", "4k3/8/8/8/3Q4/8/8/4K3 w - - 0 1");

  // 3. Ô bị tấn công
  Position p;
  parseFen("4k3/8/8/8/3Q4/8/8/4K3 w - - 0 1", p);
  printf("d8 attacked by white? %s (expect yes: hậu d4 nhìn thẳng lên)\n",
         squareAttacked(p, squareIndex("d8"), 'w') ? "yes" : "no");
  printf("e7 attacked by white? %s (expect no)\n",
         squareAttacked(p, squareIndex("e7"), 'w') ? "yes" : "no");
  printf("black in check? %s (expect no)\n", inCheck(p, 'b') ? "yes" : "no");

  parseFen("4k3/8/8/8/4Q3/8/8/4K3 b - - 0 1", p);
  printf("black in check (queen e4)? %s (expect yes)\n", inCheck(p, 'b') ? "yes" : "no");
  return 0;
}