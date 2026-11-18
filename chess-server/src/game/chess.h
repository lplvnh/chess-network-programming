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