package com.chess.chessclient.game;

/**
 * Thế cờ đọc từ chuỗi FEN mà server gửi (GAME_START, 200|MOVE, OPP_MOVE).
 *
 * Client KHÔNG kiểm tra luật cờ: server là trọng tài. Lớp này chỉ dùng để vẽ bàn cờ,
 * biết quân nào của mình, đến lượt ai, và khi nào cần hỏi quân phong cấp.
 *
 * Tọa độ: file 0..7 = cột a..h, rank 0..7 = hàng 1..8.
 * Quân: chữ hoa = Trắng (KQRBNP), chữ thường = Đen (kqrbnp), '.' = ô trống.
 */
public final class Board {
    private final char[][] sq = new char[8][8];     // [rank][file]
    private final char sideToMove;                  // 'w' hoặc 'b'
    private final String fen;

    private Board(String fen) {
        this.fen = fen;
        String[] parts = fen.trim().split(" ");
        if (parts.length < 2) throw new IllegalArgumentException("FEN thiếu trường: " + fen);

        String[] rows = parts[0].split("/");
        if (rows.length != 8) throw new IllegalArgumentException("FEN phải có 8 hàng: " + fen);
        for (int i = 0; i < 8; i++) {               // hàng đầu tiên trong FEN là hàng 8
            int rank = 7 - i, file = 0;
            for (char c : rows[i].toCharArray()) {
                if (Character.isDigit(c)) {
                    for (int k = 0; k < c - '0'; k++) set(file++, rank, '.');
                } else if ("KQRBNPkqrbnp".indexOf(c) >= 0) {
                    set(file++, rank, c);
                } else {
                    throw new IllegalArgumentException("Ký tự lạ trong FEN: " + c);
                }
            }
            if (file != 8) throw new IllegalArgumentException("Hàng " + (rank + 1) + " không đủ 8 ô: " + fen);
        }
        if (!parts[1].equals("w") && !parts[1].equals("b"))
            throw new IllegalArgumentException("Bên đi phải là w/b: " + fen);
        sideToMove = parts[1].charAt(0);
    }

    private void set(int file, int rank, char c) {
        if (file > 7) throw new IllegalArgumentException("Hàng quá 8 ô: " + fen);
        sq[rank][file] = c;
    }

    public static Board fromFen(String fen) {
        return new Board(fen);
    }

    public String fen()            { return fen; }
    public char sideToMove()       { return sideToMove; }
    public boolean whiteToMove()   { return sideToMove == 'w'; }

    /** Quân ở ô (file, rank), '.' nếu trống. */
    public char at(int file, int rank) { return sq[rank][file]; }

    public boolean isEmpty(int file, int rank) { return at(file, rank) == '.'; }

    /** Ô có quân của bên white (true) / black (false)? */
    public boolean isOwn(int file, int rank, boolean white) {
        char p = at(file, rank);
        return p != '.' && Character.isUpperCase(p) == white;
    }

    /** Đi từ (fromFile, fromRank) tới hàng toRank có phải Tốt lên hàng cuối không → cần ký tự phong cấp. */
    public boolean isPromotion(int fromFile, int fromRank, int toRank) {
        char p = at(fromFile, fromRank);
        return (p == 'P' && toRank == 7) || (p == 'p' && toRank == 0);
    }

    /** Ô của Vua bên white/black: {file, rank}, null nếu không có. */
    public int[] king(boolean white) {
        char k = white ? 'K' : 'k';
        for (int r = 0; r < 8; r++)
            for (int f = 0; f < 8; f++)
                if (sq[r][f] == k) return new int[]{f, r};
        return null;
    }

    /** (4, 1) → "e2" */
    public static String square(int file, int rank) {
        return "" + (char) ('a' + file) + (char) ('1' + rank);
    }

    /** "e2" → {4, 1}; null nếu sai dạng. */
    public static int[] parseSquare(String s) {
        if (s == null || s.length() != 2) return null;
        int f = s.charAt(0) - 'a', r = s.charAt(1) - '1';
        if (f < 0 || f > 7 || r < 0 || r > 7) return null;
        return new int[]{f, r};
    }
}
