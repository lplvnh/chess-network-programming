#include "chess.h"
#include <cctype>  // isupper, isdigit, tolower, toupper
#include <cstdio>  // printf
#include <cstdlib> // abs
#include <sstream> // istringstream

// Chỉ số trong mảng castle[]
static const int WK = 0, WQ = 1, BK = 2, BQ = 3;

static int rowOf(int sq) { return sq / 8; }
static int colOf(int sq) { return sq % 8; }
static int sqOf(int r, int c) { return r * 8 + c; }
static bool onBoard(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }

// Quân thuộc bên nào: 'w', 'b', hoặc 0 nếu ô trống
static char colorOf(char piece)
{
  if (piece == '.')
    return 0;
  return isupper((unsigned char)piece) ? 'w' : 'b';
}

static char other(char side) { return side == 'w' ? 'b' : 'w'; }

// "e2" → 52.  Cột: 'e'-'a' = 4.  Hàng: số 2 → hàng thứ 6 từ trên (8-2=6).  6*8+4 = 52
int squareIndex(const std::string &name)
{
  if (name.size() != 2)
    return -1;
  int c = name[0] - 'a';
  int r = 8 - (name[1] - '0');
  if (!onBoard(r, c))
    return -1;
  return sqOf(r, c);
}

// 52 → "e2"
std::string squareName(int sq)
{
  std::string s;
  s += char('a' + colOf(sq));
  s += char('0' + 8 - rowOf(sq));
  return s;
}

bool parseFen(const std::string &fen, Position &p)
{
  // Tách 6 phần cách nhau bởi dấu cách
  std::istringstream in(fen);
  std::string placement, side, castling, ep;
  if (!(in >> placement >> side >> castling >> ep >> p.halfmove >> p.fullmove))
    return false;

  // Phần 1: vị trí quân
  int r = 0, c = 0;
  for (char ch : placement)
  {
    if (ch == '/')
    { // xuống hàng
      if (c != 8)
        return false; // hàng trước chưa đủ 8 ô → FEN hỏng
      r++;
      c = 0;
    }
    else if (isdigit((unsigned char)ch))
    { // chữ số = số ô trống
      for (int i = 0; i < ch - '0'; i++)
      {
        if (c >= 8)
          return false;
        p.board[sqOf(r, c++)] = '.';
      }
    }
    else
    { // một quân cờ
      if (std::string("PNBRQKpnbrqk").find(ch) == std::string::npos || c >= 8)
        return false;
      p.board[sqOf(r, c++)] = ch;
    }
  }
  if (r != 7 || c != 8)
    return false; // phải đủ đúng 8 hàng × 8 ô

  // Phần 2: lượt đi
  if (side != "w" && side != "b")
    return false;
  p.side = side[0];

  // Phần 3: quyền nhập thành
  p.castle[WK] = castling.find('K') != std::string::npos;
  p.castle[WQ] = castling.find('Q') != std::string::npos;
  p.castle[BK] = castling.find('k') != std::string::npos;
  p.castle[BQ] = castling.find('q') != std::string::npos;

  // Phần 4: ô bắt tốt qua đường
  p.ep = (ep == "-") ? -1 : squareIndex(ep);
  return true;
}

std::string toFen(const Position &p)
{
  std::string s;
  // Phần 1: duyệt từng hàng, đếm ô trống liền nhau
  for (int r = 0; r < 8; r++)
  {
    int empty = 0;
    for (int c = 0; c < 8; c++)
    {
      char piece = p.board[sqOf(r, c)];
      if (piece == '.')
      {
        empty++;
        continue;
      }
      if (empty)
      {
        s += char('0' + empty);
        empty = 0;
      } // xả số ô trống đang đếm
      s += piece;
    }
    if (empty)
      s += char('0' + empty); // ô trống ở cuối hàng
    if (r < 7)
      s += '/';
  }
  // Phần 2–6
  s += ' ';
  s += p.side;
  s += ' ';
  std::string cs;
  if (p.castle[WK])
    cs += 'K';
  if (p.castle[WQ])
    cs += 'Q';
  if (p.castle[BK])
    cs += 'k';
  if (p.castle[BQ])
    cs += 'q';
  s += cs.empty() ? "-" : cs;
  s += ' ';
  s += (p.ep == -1) ? "-" : squareName(p.ep);
  s += ' ' + std::to_string(p.halfmove) + ' ' + std::to_string(p.fullmove);
  return s;
}

void printBoard(const Position &p)
{
  for (int r = 0; r < 8; r++)
  {
    printf("%d  ", 8 - r);
    for (int c = 0; c < 8; c++)
      printf("%c ", p.board[sqOf(r, c)]);
    printf("\n");
  }
  printf("\n   a b c d e f g h\n");
}