package com.chess.chessclient.ui;

import com.chess.chessclient.game.Board;
import javafx.geometry.Pos;
import javafx.scene.control.ChoiceDialog;
import javafx.scene.control.Label;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.StackPane;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.Text;

import java.util.List;
import java.util.Optional;
import java.util.function.Consumer;

/**
 * Bàn cờ 8×8 vẽ từ FEN.
 *
 * - Xoay bàn theo màu quân của mình (cầm Đen thì hàng 8 ở dưới).
 * - Bấm quân của mình rồi bấm ô đến → tạo nước đi UCI (e2e4); Tốt lên hàng cuối thì hỏi quân phong cấp (e7e8q).
 * - Không tự kiểm tra luật: nước đi gửi lên server, server trả 200 (hợp lệ) hoặc 406 (phạm luật).
 */
public class BoardView extends GridPane {
    private static final double CELL = 66;
    private static final String LIGHT = "#f0d9b5", DARK = "#b58863";
    private static final String LAST_LIGHT = "#cdd26a", LAST_DARK = "#aaa23a";
    private static final String SELECTED = "#7fb3d5", CHECK = "#e57373";
    // Dùng glyph "đặc" cho cả hai màu, tô trắng/đen; ︎ ép Windows hiển thị dạng chữ thay vì emoji
    private static final String GLYPHS = "kqrbnp";
    private static final String[] SYMBOL = {"♚", "♛", "♜", "♝", "♞", "♟"};
    private static final Font PIECE_FONT = Font.font("Segoe UI Symbol", CELL * 0.72);

    private final StackPane[][] cells = new StackPane[8][8];   // [file][rank]
    private final Text[][] pieces = new Text[8][8];

    private Board board;
    private boolean myWhite = true;
    private boolean interactive;
    private int selFile = -1, selRank = -1;
    private int[] lastFrom, lastTo;
    private boolean showCheck;
    private Consumer<String> onMove = uci -> { };

    public BoardView() {
        setAlignment(Pos.CENTER);
        build();
    }

    public void setOnMove(Consumer<String> onMove) { this.onMove = onMove; }

    /** Màu quân của mình; cũng quyết định chiều bàn cờ. */
    public void setMyColor(boolean white) {
        myWhite = white;
        build();
        refresh();
    }

    public void setBoard(Board b) {
        board = b;
        clearSelection();
    }

    /** Tô hai ô của nước vừa đi, ví dụ "e2e4". */
    public void setLastMove(String uci) {
        lastFrom = uci == null || uci.length() < 4 ? null : Board.parseSquare(uci.substring(0, 2));
        lastTo = uci == null || uci.length() < 4 ? null : Board.parseSquare(uci.substring(2, 4));
        refresh();
    }

    /** Tô đỏ Vua của bên đến lượt khi đang bị chiếu (flag CHECK). */
    public void setCheck(boolean check) {
        showCheck = check;
        refresh();
    }

    public void setInteractive(boolean on) {
        interactive = on;
        if (!on) clearSelection();
        setCursor(on ? javafx.scene.Cursor.HAND : javafx.scene.Cursor.DEFAULT);
    }

    public void clearSelection() {
        selFile = selRank = -1;
        refresh();
    }

    // ---------------- dựng lưới ----------------

    private void build() {
        getChildren().clear();
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                int file = myWhite ? col : 7 - col;
                int rank = myWhite ? 7 - row : row;
                StackPane cell = new StackPane();
                cell.setPrefSize(CELL, CELL);
                cell.setMinSize(CELL, CELL);
                Text t = new Text();
                t.setFont(PIECE_FONT);
                t.setStrokeWidth(1.2);
                cell.getChildren().add(t);
                final int f = file, r = rank;
                cell.setOnMouseClicked(e -> onClick(f, r));
                cells[file][rank] = cell;
                pieces[file][rank] = t;
                add(cell, col + 1, row);
            }
            Label rankLabel = coord(String.valueOf(myWhite ? 8 - row : row + 1));
            rankLabel.setMinWidth(18);
            add(rankLabel, 0, row);
        }
        for (int col = 0; col < 8; col++) {
            char fileChar = (char) ('a' + (myWhite ? col : 7 - col));
            Label l = coord(String.valueOf(fileChar));
            l.setMinWidth(CELL);
            add(l, col + 1, 8);
        }
    }

    private static Label coord(String s) {
        Label l = new Label(s);
        l.setAlignment(Pos.CENTER);
        l.setStyle("-fx-text-fill: #555; -fx-font-size: 12px;");
        return l;
    }

    // ---------------- vẽ ----------------

    private void refresh() {
        int[] king = (board != null && showCheck) ? board.king(board.whiteToMove()) : null;
        for (int f = 0; f < 8; f++) {
            for (int r = 0; r < 8; r++) {
                boolean light = (f + r) % 2 == 1;
                String bg = light ? LIGHT : DARK;
                if (isAt(lastFrom, f, r) || isAt(lastTo, f, r)) bg = light ? LAST_LIGHT : LAST_DARK;
                if (isAt(king, f, r)) bg = CHECK;
                if (f == selFile && r == selRank) bg = SELECTED;
                cells[f][r].setStyle("-fx-background-color: " + bg + ";");

                Text t = pieces[f][r];
                char p = board == null ? '.' : board.at(f, r);
                if (p == '.') {
                    t.setText("");
                } else {
                    t.setText(SYMBOL[GLYPHS.indexOf(Character.toLowerCase(p))] + "︎");
                    boolean white = Character.isUpperCase(p);
                    t.setFill(white ? Color.WHITE : Color.web("#222"));
                    t.setStroke(white ? Color.web("#333") : Color.web("#000"));
                }
            }
        }
    }

    private static boolean isAt(int[] sq, int f, int r) {
        return sq != null && sq[0] == f && sq[1] == r;
    }

    // ---------------- bấm chuột ----------------

    private void onClick(int file, int rank) {
        if (!interactive || board == null) return;
        if (selFile < 0) {
            if (board.isOwn(file, rank, myWhite)) { selFile = file; selRank = rank; }
        } else if (file == selFile && rank == selRank) {
            selFile = selRank = -1;                                  // bấm lại: bỏ chọn
        } else if (board.isOwn(file, rank, myWhite)) {
            selFile = file; selRank = rank;                          // chọn quân khác
        } else {
            String uci = Board.square(selFile, selRank) + Board.square(file, rank);
            if (board.isPromotion(selFile, selRank, rank)) {
                Optional<String> promo = askPromotion();
                if (promo.isEmpty()) { clearSelection(); return; }
                uci += promo.get();
            }
            selFile = selRank = -1;
            refresh();
            onMove.accept(uci);
            return;
        }
        refresh();
    }

    private Optional<String> askPromotion() {
        List<String> names = List.of("Hậu", "Xe", "Tượng", "Mã");
        ChoiceDialog<String> d = new ChoiceDialog<>("Hậu", names);
        d.setTitle("Phong cấp");
        d.setHeaderText("Tốt lên hàng cuối. Phong thành quân nào?");
        d.setContentText("Quân:");
        if (getScene() != null) d.initOwner(getScene().getWindow());
        return d.showAndWait().map(n -> switch (n) {
            case "Xe" -> "r";
            case "Tượng" -> "b";
            case "Mã" -> "n";
            default -> "q";
        });
    }
}
