// Chess engine: luật cờ vua, không phụ thuộc mạng
#pragma once
#include <string>
#include <vector>

// Một thế cờ = đúng 6 phần của FEN
struct Position
{
  char board[64]; // 'P','n',... hoặc '.' cho ô trống
  char side;      // 'w' hoặc 'b': đến lượt ai
  bool castle[4]; // quyền nhập thành: [0]=K [1]=Q [2]=k [3]=q
  int ep;         // ô bắt tốt qua đường, -1 nếu không có
  int halfmove;   // đếm cho luật 50 nước
  int fullmove;   // số thứ tự nước
};

bool parseFen(const std::string &fen, Position &p); // chuỗi FEN → Position (false nếu sai)
std::string toFen(const Position &p);               // Position → chuỗi FEN
void printBoard(const Position &p);                 // in ra terminal để debug

int squareIndex(const std::string &name); // "e2" → 52
std::string squareName(int sq);           // 52 → "e2"

struct Move
{
  int from;
  int to;
  char promo; // 'q','r','b','n' khi phong cấp, 0 nếu không
};

std::string moveToUci(const Move &m);                        // {52,36,0} → "e2e4"
bool squareAttacked(const Position &p, int sq, char bySide); // ô sq có bị bySide tấn công?
bool inCheck(const Position &p, char side);                  // vua của side có bị chiếu?
std::vector<Move> legalMoves(const Position &p);             // mọi nước đi hợp lệ

Position makeMove(const Position &p, const Move &m); // trả về thế cờ SAU khi đi
long long perft(const Position &p, int depth);       // đếm để test