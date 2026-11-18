package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.game.Board;
import com.chess.chessclient.net.Message;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.Alert;
import javafx.scene.control.Button;
import javafx.scene.control.ButtonType;
import javafx.scene.control.Label;
import javafx.scene.control.ListView;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.VBox;
import javafx.stage.Modality;

import java.util.ArrayList;
import java.util.List;

/**
 * Màn hình ván cờ (IN_GAME), mở khi nhận GAME_START|room|source|color|opp|opp_rating|fen.
 *
 *   UC10 MOVE|uci        → 200|MOVE|uci|flag|fen | 406 ILLEGAL | 407 NOT_YOUR_TURN | 400 BAD_FORMAT
 *        event OPP_MOVE|uci|flag|fen
 *   UC11 event GAME_OVER|result|reason|new_rating|delta
 *   UC12 RESIGN          → 200|RESIGN, sau đó GAME_OVER
 *   UC13 OFFER_DRAW      → 200 | 405 ALREADY_PENDING | 405 WAIT_YOUR_MOVE;   event DRAW_OFFERED / DRAW_DECLINED
 *        ACCEPT_DRAW / DECLINE_DRAW → 200 | 404 NO_OFFER
 *
 * Bàn cờ luôn vẽ lại từ FEN server gửi, nên không bao giờ lệch với thế cờ chính thức.
 */
public class GameView implements Screen {
    private final ChessApp app;
    private final BorderPane root = new BorderPane();
    private final BoardView boardView = new BoardView();

    private final String roomId;
    private final boolean white;
    private final String opp;
    private final int oppRating;

    private Board board;
    private boolean waitingMoveReply;      // đã gửi MOVE, chưa nhận 200/406/407
    private boolean over;
    private final List<String> moves = new ArrayList<>();

    private final Label header = new Label();
    private final Label turnLabel = new Label();
    private final Label status = new Label();
    private final ListView<String> moveList = new ListView<>();
    private final HBox drawBar;
    private final Button drawBtn = new Button("Cầu hòa");
    private final Button resignBtn = new Button("Xin thua");
    private final Button lobbyBtn = new Button("Về sảnh");

    public GameView(ChessApp app, Message start) {
        this.app = app;
        // GAME_START|room|source|color|opp|opp_rating|fen
        this.roomId = start.arg(0);
        String source = start.arg(1);
        this.white = start.arg(2).equals("WHITE");
        this.opp = start.arg(3);
        this.oppRating = start.argInt(4, 0);
        this.board = Board.fromFen(start.arg(5));

        // ----- bàn cờ -----
        boardView.setMyColor(white);
        boardView.setBoard(board);
        boardView.setOnMove(this::sendMove);
        VBox boardBox = new VBox(boardView);
        boardBox.setAlignment(Pos.CENTER);
        boardBox.setPadding(new Insets(16));

        // ----- cột phải -----
        header.setStyle("-fx-font-size: 16px; -fx-font-weight: bold;");
        header.setText("Phòng " + roomId + " · " + Texts.source(source));
        Label oppLabel = new Label((white ? "♚ " : "♔ ") + opp + " (" + oppRating + ")  — đối thủ");
        Label meLabel = new Label((white ? "♔ " : "♚ ") + app.user() + " (" + app.rating() + ")  — bạn cầm "
                + (white ? "Trắng" : "Đen"));
        oppLabel.setStyle("-fx-font-size: 14px;");
        meLabel.setStyle("-fx-font-size: 14px;");
        turnLabel.setStyle("-fx-font-size: 18px; -fx-font-weight: bold;");
        status.setWrapText(true);
        status.setMinHeight(40);

        Label drawText = new Label("Đối thủ đề nghị hòa");
        Button acceptDraw = new Button("Đồng ý");
        Button declineDraw = new Button("Từ chối");
        acceptDraw.setOnAction(e -> app.send("ACCEPT_DRAW"));
        declineDraw.setOnAction(e -> app.send("DECLINE_DRAW"));
        drawBar = new HBox(8, drawText, acceptDraw, declineDraw);
        drawBar.setAlignment(Pos.CENTER_LEFT);
        drawBar.setPadding(new Insets(8));
        drawBar.setStyle("-fx-background-color: #fff3cd; -fx-border-color: #e0c060; -fx-background-radius: 4; -fx-border-radius: 4;");
        showDrawBar(false);

        drawBtn.setOnAction(e -> app.send("OFFER_DRAW"));
        resignBtn.setOnAction(e -> confirmResign());
        lobbyBtn.setOnAction(e -> app.show(new LobbyView(app)));
        lobbyBtn.setVisible(false);
        lobbyBtn.setManaged(false);
        HBox actions = new HBox(10, drawBtn, resignBtn, lobbyBtn);

        moveList.setPrefHeight(220);
        VBox side = new VBox(12, header, oppLabel, meLabel, turnLabel, drawBar, status,
                new Label("Các nước đã đi"), moveList, actions);
        VBox.setVgrow(moveList, Priority.ALWAYS);
        side.setPadding(new Insets(16));
        side.setPrefWidth(320);

        root.setCenter(boardBox);
        root.setRight(side);
        update();
    }

    @Override
    public Parent root() {
        return root;
    }

    // ---------------- thao tác người dùng ----------------

    private void sendMove(String uci) {
        waitingMoveReply = true;
        update();
        app.send("MOVE", uci);
    }

    private void confirmResign() {
        Alert a = new Alert(Alert.AlertType.CONFIRMATION, "Bạn chắc chắn muốn xin thua?",
                ButtonType.YES, ButtonType.NO);
        a.setHeaderText(null);
        a.setTitle("Xin thua");
        a.initOwner(app.stage());
        a.showAndWait().filter(b -> b == ButtonType.YES).ifPresent(b -> {
            if (!over) app.send("RESIGN");
        });
    }

    // ---------------- cập nhật giao diện ----------------

    private boolean myTurn() {
        return board.whiteToMove() == white;
    }

    private void update() {
        boolean canMove = !over && !waitingMoveReply && myTurn();
        boardView.setInteractive(canMove);
        drawBtn.setDisable(over);
        resignBtn.setDisable(over);
        if (over) turnLabel.setText("Ván đã kết thúc");
        else if (waitingMoveReply) turnLabel.setText("Đang gửi nước đi...");
        else turnLabel.setText(myTurn() ? "Đến lượt bạn" : "Đến lượt đối thủ");
        turnLabel.setStyle("-fx-font-size: 18px; -fx-font-weight: bold; -fx-text-fill: "
                + (canMove ? "#1e7e34" : "#555") + ";");
    }

    private void applyMove(String uci, String flag, String fen) {
        board = Board.fromFen(fen);
        boardView.setBoard(board);
        boardView.setLastMove(uci);
        boardView.setCheck("CHECK".equals(flag));
        moves.add(uci);
        renderMoves();
    }

    private void renderMoves() {
        List<String> lines = new ArrayList<>();
        for (int i = 0; i < moves.size(); i += 2) {
            String w = moves.get(i);
            String b = i + 1 < moves.size() ? moves.get(i + 1) : "";
            lines.add((i / 2 + 1) + ".  " + w + "   " + b);
        }
        moveList.getItems().setAll(lines);
        if (!lines.isEmpty()) moveList.scrollTo(lines.size() - 1);
    }

    private void showDrawBar(boolean on) {
        drawBar.setVisible(on);
        drawBar.setManaged(on);
    }

    private void info(String text) {
        status.setStyle("-fx-text-fill: #1f4e79;");
        status.setText(text);
    }

    private void error(String text) {
        status.setStyle("-fx-text-fill: #c0392b;");
        status.setText(text);
    }

    // ---------------- bản tin ----------------

    @Override
    public void onMessage(Message m) {
        if (m.isEvent()) {
            onEvent(m);
            return;
        }
        switch (m.name()) {
            case "MOVE" -> {
                waitingMoveReply = false;
                if (m.isOk()) {
                    // 200|MOVE|uci|flag|fen. Mình đi quân = từ chối lời cầu hòa của đối thủ (nếu có)
                    applyMove(m.arg(0), m.arg(1), m.arg(2));
                    showDrawBar(false);
                    info("CHECK".equals(m.arg(1)) ? "Chiếu!" : "");
                } else {
                    boardView.clearSelection();
                    error(Texts.error(m));                // 406 / 407 / 400: lượt giữ nguyên, đi lại
                }
                update();
            }
            case "OFFER_DRAW" -> {
                if (m.isOk()) info("Đã gửi lời cầu hòa, chờ đối thủ trả lời");
                else error(Texts.error(m));
            }
            case "ACCEPT_DRAW", "DECLINE_DRAW" -> {
                showDrawBar(false);
                if (!m.isOk()) error(Texts.error(m));
                else if (m.name().equals("DECLINE_DRAW")) info("Bạn đã từ chối hòa");
            }
            case "RESIGN" -> {
                if (!m.isOk()) error(Texts.error(m));     // 200: chờ GAME_OVER
            }
            default -> {
                if (!m.isOk()) error(Texts.error(m));
            }
        }
    }

    private void onEvent(Message m) {
        switch (m.name()) {
            case "OPP_MOVE" -> {                          // OPP_MOVE|uci|flag|fen
                applyMove(m.arg(0), m.arg(1), m.arg(2));
                if ("CHECK".equals(m.arg(1))) error("Bạn đang bị chiếu!");
                else info("");
                update();
            }
            case "DRAW_OFFERED" -> {
                showDrawBar(true);
                info("Đối thủ đề nghị hòa. Bạn có thể đồng ý, từ chối, hoặc đi tiếp (coi như từ chối).");
            }
            case "DRAW_DECLINED" -> info("Đối thủ từ chối hòa");
            case "GAME_OVER" -> onGameOver(m);
            default -> { }
        }
    }

    /** GAME_OVER|result|reason|new_rating|delta: ván kết thúc, server đã đưa mình về sảnh (IDLE). */
    private void onGameOver(Message m) {
        over = true;
        waitingMoveReply = false;
        showDrawBar(false);
        String result = m.arg(0), reason = m.arg(1), delta = m.arg(3);
        int newRating = m.argInt(2, app.rating());
        app.setRating(newRating);

        String text = Texts.result(result) + "\n" + Texts.reason(reason)
                + "\nRating mới: " + newRating + " (" + delta + ")";
        status.setStyle("-fx-font-weight: bold; -fx-text-fill: "
                + (result.equals("WIN") ? "#1e7e34" : result.equals("LOSS") ? "#c0392b" : "#1f4e79") + ";");
        status.setText(text.replace("\n", " · "));
        drawBtn.setVisible(false);
        drawBtn.setManaged(false);
        resignBtn.setVisible(false);
        resignBtn.setManaged(false);
        lobbyBtn.setVisible(true);
        lobbyBtn.setManaged(true);
        update();

        // Hộp thoại không chặn: vẫn nhìn được thế cờ cuối, vẫn nhận được lời mời mới
        Alert a = new Alert(Alert.AlertType.INFORMATION, text);
        a.setTitle("Kết thúc ván");
        a.setHeaderText(Texts.result(result));
        a.initOwner(app.stage());
        a.initModality(Modality.NONE);
        a.show();
    }
}
