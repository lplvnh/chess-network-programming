package com.chess.chessclient.ui;

import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.ChoiceDialog;
import javafx.scene.layout.GridPane;

import java.util.Map;
import java.util.Optional;
import java.util.function.Consumer;

// Bàn cờ 8x8: vẽ từ FEN, cho người chơi chọn nước đi, báo ra ngoài qua onMove
public class BoardView extends GridPane {
    private static final Map<Character, String> GLYPH = Map.ofEntries(
            Map.entry('K', "♔"), Map.entry('Q', "♕"), Map.entry('R', "♖"),
            Map.entry('B', "♗"), Map.entry('N', "♘"), Map.entry('P', "♙"),
            Map.entry('k', "♚"), Map.entry('q', "♛"), Map.entry('r', "♜"),
            Map.entry('b', "♝"), Map.entry('n', "♞"), Map.entry('p', "♟"));

    private final Button[] cells = new Button[64];   // cells[ô] = nút hiển thị ô đó
    private final char[] board = new char[64];       // '.' hoặc ký tự quân, giống server
    private boolean flipped = false;                 // true = nhìn từ phía đen
    private boolean enabled = false;
    private char myColor = 'w';
    private int selected = -1;                       // ô đang chọn, -1 = chưa chọn
    private int lastFrom = -1, lastTo = -1;          // nước vừa đi (tô sáng)
    private Consumer<String> onMove = uci -> { };

    public BoardView() {
        for (int sq = 0; sq < 64; sq++) {
            Button b = new Button();
            b.setPrefSize(64, 64);
            b.setMinSize(64, 64);
            b.setAlignment(Pos.CENTER);
            final int square = sq;                   // lambda cần biến không đổi
            b.setOnAction(e -> onSquareClicked(square));
            cells[sq] = b;
        }
        layoutCells();
    }

    // ----- API cho GameView -----
    public void setOnMove(Consumer<String> handler) { onMove = handler; }

    public void setMyColor(String color) {           // "WHITE" / "BLACK"
        myColor = color.equals("WHITE") ? 'w' : 'b';
        flipped = myColor == 'b';
        layoutCells();
    }

    public void setEnabled(boolean on) {
        enabled = on;
        if (!on) selected = -1;
        repaint();
    }

    public void setLastMove(String uci) {            // "e2e4" → tô sáng e2 và e4
        lastFrom = squareIndex(uci.substring(0, 2));
        lastTo   = squareIndex(uci.substring(2, 4));
        repaint();
    }

    // Đọc phần 1 của FEN vào mảng board (cùng cách parseFen bên server)
    public void setFen(String fen) {
        String placement = fen.split(" ")[0];
        int sq = 0;
        for (char ch : placement.toCharArray()) {
            if (ch == '/') continue;
            if (Character.isDigit(ch)) {
                for (int i = 0; i < ch - '0'; i++) board[sq++] = '.';
            } else {
                board[sq++] = ch;
            }
        }
        selected = -1;
        repaint();
    }

    // ----- Bố trí: đặt 64 nút vào lưới, có xoay nếu cầm đen -----
    private void layoutCells() {
        getChildren().clear();
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                int sq = flipped ? (7 - r) * 8 + (7 - c) : r * 8 + c;
                add(cells[sq], c, r);                // GridPane.add(node, cột, hàng)
            }
        }
    }

    // ----- Vẽ lại màu ô và quân -----
    private void repaint() {
        for (int sq = 0; sq < 64; sq++) {
            int r = sq / 8, c = sq % 8;
            String bg = ((r + c) % 2 == 0) ? "#f0d9b5" : "#b58863";   // ô sáng / ô tối
            if (sq == lastFrom || sq == lastTo) bg = "#cdd26a";        // nước vừa đi
            if (sq == selected) bg = "#f6f669";                        // ô đang chọn
            cells[sq].setText(board[sq] == '.' ? "" : GLYPH.get(board[sq]));
            cells[sq].setStyle("-fx-background-color: " + bg + "; -fx-background-radius: 0;"
                    + "-fx-font-size: 36px; -fx-padding: 0;");
        }
    }

    // ----- Xử lý bấm ô -----
    private void onSquareClicked(int sq) {
        if (!enabled) return;
        char piece = board[sq];
        boolean mine = piece != '.' && (Character.isUpperCase(piece) ? 'w' : 'b') == myColor;

        if (selected == -1) {                         // lần bấm 1: phải chọn quân của mình
            if (mine) selected = sq;
        } else if (sq == selected) {                  // bấm lại chính nó: bỏ chọn
            selected = -1;
        } else if (mine) {                            // bấm quân khác của mình: đổi lựa chọn
            selected = sq;
        } else {                                      // lần bấm 2: ô đích → tạo nước đi
            String uci = squareName(selected) + squareName(sq);
            if (isPromotion(selected, sq)) {
                Optional<String> promo = askPromotion();
                if (promo.isEmpty()) { selected = -1; repaint(); return; }   // bấm Hủy
                uci += promo.get();
            }
            selected = -1;
            repaint();
            onMove.accept(uci);                       // báo cho GameView gửi đi
            return;
        }
        repaint();
    }

    // Tốt đi tới hàng cuối? (hàng 0 = rank 8, hàng 7 = rank 1)
    private boolean isPromotion(int from, int to) {
        char p = board[from];
        int toRow = to / 8;
        return (p == 'P' && toRow == 0) || (p == 'p' && toRow == 7);
    }

    private Optional<String> askPromotion() {
        ChoiceDialog<String> dlg = new ChoiceDialog<>("Hậu", "Hậu", "Xe", "Tượng", "Mã");
        dlg.setTitle("Phong cấp");
        dlg.setHeaderText("Chọn quân để phong cấp");
        return dlg.showAndWait().map(choice -> switch (choice) {
            case "Xe"    -> "r";
            case "Tượng" -> "b";
            case "Mã"    -> "n";
            default      -> "q";
        });
    }

    // Đổi qua lại giữa số ô và tên ô (giống server)
    private static String squareName(int sq) {
        return "" + (char) ('a' + sq % 8) + (char) ('0' + 8 - sq / 8);
    }

    private static int squareIndex(String name) {
        return (8 - (name.charAt(1) - '0')) * 8 + (name.charAt(0) - 'a');
    }
}