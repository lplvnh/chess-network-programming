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

  // 4. Lọc nước đi: đen bị hậu e4 chiếu, vua e8 không được bước sang e7
  listMoves("Black in check", "4k3/8/8/8/4Q3/8/8/4K3 b - - 0 1");

  // 5. perft từ thế cờ ban đầu
  const char *START = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  long long expected[] = {1, 20, 400, 8902, 197281};
  parseFen(START, p);
  for (int d = 1; d <= 4; d++)
  {
    long long got = perft(p, d);
    printf("perft(%d) = %8lld  expected %8lld  %s\n", d, got, expected[d],
           got == expected[d] ? "OK" : "FAIL");
  }
  // 6. Thế cờ khó: kiểm tra phong cấp, bắt qua đường, nhập thành
  struct Case
  {
    const char *name;
    const char *fen;
    int depth;
    long long expected;
  };
  Case hard[] = {
      {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862},
      {"pos3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238},
      {"pos4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467},
      {"pos5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379},
  };
  for (const Case &t : hard)
  {
    parseFen(t.fen, p);
    long long got = perft(p, t.depth);
    printf("%-9s depth %d = %7lld  expected %7lld  %s\n", t.name, t.depth, got, t.expected,
           got == t.expected ? "OK" : "FAIL");
  }

  // 7. Nhìn tận mắt: nhập thành
  listMoves("Castling", "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
  return 0;
}