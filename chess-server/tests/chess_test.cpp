#include "chess.h"
#include <cstdio>

int main()
{
  std::string start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  Position p;
  if (!parseFen(start, p))
  {
    printf("parseFen FAILED\n");
    return 1;
  }

  printBoard(p);

  std::string back = toFen(p);
  printf("\nFEN in : %s\nFEN out: %s\n%s\n", start.c_str(), back.c_str(),
         back == start ? "ROUND-TRIP OK" : "ROUND-TRIP FAIL");

  printf("e2 = %d, 52 = %s\n", squareIndex("e2"), squareName(52).c_str());

  Position bad;
  printf("FEN hỏng: %s\n", parseFen("rnbqkbnr/ppp w KQkq - 0 1", bad) ? "nhận nhầm" : "bị từ chối (đúng)");
  return 0;
}