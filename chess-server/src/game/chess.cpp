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

std::string moveToUci(const Move &m)
{
  std::string s = squareName(m.from) + squareName(m.to);
  if (m.promo)
    s += m.promo;
  return s;
}

// Bảng hướng (đổi hàng, đổi cột)
static const int KNIGHT[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
static const int KING[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
static const int ROOK_DIRS[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
static const int BISHOP_DIRS[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

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

bool squareAttacked(const Position &p, int sq, char by)
{
  int r = rowOf(sq), c = colOf(sq);

  // Hàm nhỏ: ô (rr,cc) có quân loại 'lowerPiece' của bên 'by' không?
  auto is = [&](int rr, int cc, char lowerPiece)
  {
    if (!onBoard(rr, cc))
      return false;
    char piece = p.board[sqOf(rr, cc)];
    return colorOf(piece) == by && tolower((unsigned char)piece) == lowerPiece;
  };

  // Tốt: tốt TRẮNG tấn công LÊN (hàng nhỏ hơn), nên nó phải đứng ở hàng r+1
  int pr = (by == 'w') ? r + 1 : r - 1;
  if (is(pr, c - 1, 'p') || is(pr, c + 1, 'p'))
    return true;

  // Mã, vua: nhìn ra 8 ô
  for (auto &d : KNIGHT)
    if (is(r + d[0], c + d[1], 'n'))
      return true;
  for (auto &d : KING)
    if (is(r + d[0], c + d[1], 'k'))
      return true;

  // Xe/hậu: trượt thẳng, chỉ xét quân ĐẦU TIÊN gặp được
  for (auto &d : ROOK_DIRS)
  {
    int rr = r + d[0], cc = c + d[1];
    while (onBoard(rr, cc))
    {
      char piece = p.board[sqOf(rr, cc)];
      if (piece != '.')
      {
        if (colorOf(piece) == by && (tolower(piece) == 'r' || tolower(piece) == 'q'))
          return true;
        break; // bị chắn → hướng này xong
      }
      rr += d[0];
      cc += d[1];
    }
  }
  // Tượng/hậu: trượt chéo
  for (auto &d : BISHOP_DIRS)
  {
    int rr = r + d[0], cc = c + d[1];
    while (onBoard(rr, cc))
    {
      char piece = p.board[sqOf(rr, cc)];
      if (piece != '.')
      {
        if (colorOf(piece) == by && (tolower(piece) == 'b' || tolower(piece) == 'q'))
          return true;
        break;
      }
      rr += d[0];
      cc += d[1];
    }
  }
  return false;
}

// Tìm vua của side, hỏi xem ô đó có bị bên kia tấn công không
bool inCheck(const Position &p, char side)
{
  char king = (side == 'w') ? 'K' : 'k';
  for (int sq = 0; sq < 64; sq++)
    if (p.board[sq] == king)
      return squareAttacked(p, sq, other(side));
  return false;
}

// Trượt theo 4 hướng trong dirs (dùng cho xe, tượng, hậu)
static void slide(const Position &p, std::vector<Move> &moves, int from, const int dirs[4][2])
{
  int r = rowOf(from), c = colOf(from);
  for (int i = 0; i < 4; i++)
  {
    int rr = r + dirs[i][0], cc = c + dirs[i][1];
    while (onBoard(rr, cc))
    {
      char target = p.board[sqOf(rr, cc)];
      if (colorOf(target) == p.side)
        break;                                  // quân mình chắn → dừng
      moves.push_back({from, sqOf(rr, cc), 0}); // ô trống hoặc quân địch
      if (target != '.')
        break; // ăn quân địch → dừng
      rr += dirs[i][0];
      cc += dirs[i][1];
    }
  }
}

// Thêm nước đi của tốt; nếu tới hàng cuối thì thêm 4 lựa chọn phong cấp
static void addPawnMove(std::vector<Move> &moves, int from, int to)
{
  int r = rowOf(to);
  if (r == 0 || r == 7)
  {
    for (char promo : {'q', 'r', 'b', 'n'})
      moves.push_back({from, to, promo});
  }
  else
  {
    moves.push_back({from, to, 0});
  }
}

// Mọi nước đi "theo hình dạng" của bên đang tới lượt (chưa xét vua bị chiếu)
static std::vector<Move> pseudoMoves(const Position &p)
{
  std::vector<Move> moves;
  char me = p.side;
  for (int from = 0; from < 64; from++)
  {
    char piece = p.board[from];
    if (colorOf(piece) != me)
      continue; // chỉ xét quân của mình
    int r = rowOf(from), c = colOf(from);

    switch (tolower((unsigned char)piece))
    {
    case 'p':
    {
      int dir = (me == 'w') ? -1 : 1;     // trắng đi lên, đen đi xuống
      int startRow = (me == 'w') ? 6 : 1; // hàng xuất phát của tốt
      // Tiến 1 ô (ô phía trước phải trống)
      if (onBoard(r + dir, c) && p.board[sqOf(r + dir, c)] == '.')
      {
        addPawnMove(moves, from, sqOf(r + dir, c));
        // Tiến 2 ô: chỉ từ hàng xuất phát, và ô thứ 2 cũng trống
        if (r == startRow && p.board[sqOf(r + 2 * dir, c)] == '.')
          moves.push_back({from, sqOf(r + 2 * dir, c), 0});
      }
      // Ăn chéo: chỉ khi ô chéo có quân địch
      for (int dc : {-1, 1})
      {
        if (!onBoard(r + dir, c + dc))
          continue;
        int to = sqOf(r + dir, c + dc);
        if (colorOf(p.board[to]) == other(me) || to == p.ep)
          addPawnMove(moves, from, to);
      }
      break;
    }
    case 'n':
      for (auto &d : KNIGHT)
      {
        int rr = r + d[0], cc = c + d[1];
        if (onBoard(rr, cc) && colorOf(p.board[sqOf(rr, cc)]) != me)
          moves.push_back({from, sqOf(rr, cc), 0});
      }
      break;
    case 'b':
      slide(p, moves, from, BISHOP_DIRS);
      break;
    case 'r':
      slide(p, moves, from, ROOK_DIRS);
      break;
    case 'q':
      slide(p, moves, from, BISHOP_DIRS);
      slide(p, moves, from, ROOK_DIRS);
      break;
    case 'k':
      for (auto &d : KING)
      {
        int rr = r + d[0], cc = c + d[1];
        if (onBoard(rr, cc) && colorOf(p.board[sqOf(rr, cc)]) != me)
          moves.push_back({from, sqOf(rr, cc), 0});
      }
      int home = (me == 'w') ? 60 : 4; // e1 = 60, e8 = 4
      int kRight = (me == 'w') ? WK : BK;
      int qRight = (me == 'w') ? WQ : BQ;
      char opp = other(me);
      if (from == home && !squareAttacked(p, home, opp))
      { // vua ở nhà, không bị chiếu
        // Cánh vua: f, g trống và không bị tấn công
        if (p.castle[kRight] && p.board[home + 1] == '.' && p.board[home + 2] == '.' &&
            !squareAttacked(p, home + 1, opp) && !squareAttacked(p, home + 2, opp))
          moves.push_back({from, home + 2, 0});
        // Cánh hậu: d, c, b trống; d, c không bị tấn công
        if (p.castle[qRight] && p.board[home - 1] == '.' && p.board[home - 2] == '.' &&
            p.board[home - 3] == '.' &&
            !squareAttacked(p, home - 1, opp) && !squareAttacked(p, home - 2, opp))
          moves.push_back({from, home - 2, 0});
      }
      break;
    }
  }
  return moves;
}

Position makeMove(const Position &p, const Move &m)
{
  Position n = p; // sao chép, p giữ nguyên
  char piece = n.board[m.from];
  char captured = n.board[m.to]; // '.' nếu ô đích trống
  bool isPawn = tolower((unsigned char)piece) == 'p';

  // 1. Di chuyển quân (quân bị ăn tự mất vì bị ghi đè)
  n.board[m.to] = piece;
  n.board[m.from] = '.';

  // Bắt tốt qua đường: tốt bị ăn nằm CÙNG HÀNG với ô xuất phát, CÙNG CỘT với ô đích
  if (isPawn && m.to == p.ep)
    n.board[sqOf(rowOf(m.from), colOf(m.to))] = '.';

  // Phong cấp: thay tốt bằng quân mới, đúng màu
  if (m.promo)
    n.board[m.to] = (p.side == 'w') ? toupper(m.promo) : m.promo;

  // Nhập thành: vua đi 2 cột → chuyển luôn xe
  bool isKing = tolower((unsigned char)piece) == 'k';
  if (isKing && colOf(m.to) - colOf(m.from) == 2)
  { // cánh vua: xe h → f
    n.board[m.to - 1] = n.board[m.to + 1];
    n.board[m.to + 1] = '.';
  }
  if (isKing && colOf(m.from) - colOf(m.to) == 2)
  { // cánh hậu: xe a → d
    n.board[m.to + 1] = n.board[m.to - 2];
    n.board[m.to - 2] = '.';
  }

  // Cập nhật quyền nhập thành: có quân rời hoặc bị ăn tại ô vua/xe ban đầu → mất quyền
  for (int sq : {m.from, m.to})
  {
    if (sq == 60)
    {
      n.castle[WK] = n.castle[WQ] = false;
    } // e1: vua trắng
    if (sq == 4)
    {
      n.castle[BK] = n.castle[BQ] = false;
    } // e8: vua đen
    if (sq == 63)
      n.castle[WK] = false; // h1
    if (sq == 56)
      n.castle[WQ] = false; // a1
    if (sq == 7)
      n.castle[BK] = false; // h8
    if (sq == 0)
      n.castle[BQ] = false; // a8
  }

  // Ô bắt qua đường: chỉ có ngay sau khi một tốt đi 2 ô
  n.ep = -1;
  if (isPawn && abs(rowOf(m.to) - rowOf(m.from)) == 2)
    n.ep = (m.from + m.to) / 2; // ô ở giữa

  // 2. Đồng hồ 50 nước
  n.halfmove = (isPawn || captured != '.') ? 0 : p.halfmove + 1;

  // 3. Số thứ tự nước tăng sau khi đen đi
  if (p.side == 'b')
    n.fullmove++;

  // 4. Đổi lượt
  n.side = other(p.side);
  return n;
}

std::vector<Move> legalMoves(const Position &p)
{
  std::vector<Move> legal;
  for (const Move &m : pseudoMoves(p))
  {
    Position n = makeMove(p, m);
    if (!inCheck(n, p.side))
      legal.push_back(m); // p.side = bên VỪA đi
  }
  return legal;
}

long long perft(const Position &p, int depth)
{
  if (depth == 0)
    return 1; // đi hết độ sâu = 1 chuỗi hoàn chỉnh
  long long total = 0;
  for (const Move &m : legalMoves(p))
    total += perft(makeMove(p, m), depth - 1); // đệ quy: thử từng nước, đếm tiếp
  return total;
}