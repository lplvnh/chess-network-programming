package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.Alert;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;

public class GameView implements Screen {
    private final ChessApp app;
    private final HBox root = new HBox(20);
    private final BoardView board = new BoardView();

    private final String myColor;                 // "WHITE" / "BLACK"
    private final Label turnLabel = new Label();
    private final Label status = new Label();

    // msg = GAME_START|room_id|source|color|opp|opp_rating|fen
    public GameView(ChessApp app, Message msg) {
        this.app = app;
        String roomId    = msg.arg(0);
        String source    = msg.arg(1);
        this.myColor     = msg.arg(2);
        String opp       = msg.arg(3);
        String oppRating = msg.arg(4);
        String fen       = msg.arg(5);

        board.setMyColor(myColor);
        board.setFen(fen);
        board.setOnMove(uci -> {
            board.setEnabled(false);              // khóa tới khi server trả lời
            status.setText("");
            app.send("MOVE|" + uci);
        });
        updateTurn(fen);

        // Bảng thông tin bên phải
        Label title = new Label("Ván " + roomId + " · " + source);
        title.setStyle("-fx-font-size: 18px; -fx-font-weight: bold;");
        Label oppLabel = new Label("Đối thủ: " + opp + " (" + oppRating + ")");
        Label meLabel  = new Label("Bạn: " + app.myUser + " (" + app.myRating + ") · "
                + (myColor.equals("WHITE") ? "Trắng ♔" : "Đen ♚"));
        turnLabel.setStyle("-fx-font-size: 16px;");
        status.setStyle("-fx-text-fill: #c0392b; -fx-font-size: 14px;");

        Button resignBtn = new Button("Xin thua");
        resignBtn.setOnAction(e -> app.send("RESIGN"));

        VBox side = new VBox(12, title, oppLabel, meLabel, turnLabel, status, resignBtn);
        side.setPrefWidth(260);

        root.getChildren().addAll(board, side);
        root.setAlignment(Pos.CENTER);
        root.setPadding(new Insets(20));
    }

    @Override
    public Parent root() { return root; }

    @Override
    public void onMessage(Message msg) {
        switch (msg.command()) {
            case "MOVE" -> {
                if (msg.isOk()) {                         // 200|MOVE|uci|flag|fen
                    board.setFen(msg.arg(2));
                    board.setLastMove(msg.arg(0));
                    updateTurn(msg.arg(2));
                    if (msg.arg(1).equals("CHECK")) status.setText("Chiếu!");
                } else {                                  // 406 / 407 / 400
                    status.setText(switch (msg.arg(0)) {
                        case "ILLEGAL"       -> "Nước đi không hợp lệ";
                        case "NOT_YOUR_TURN" -> "Chưa đến lượt bạn";
                        default              -> "Lỗi: " + msg.arg(0);
                    });
                    board.setEnabled(msg.arg(0).equals("ILLEGAL"));   // sai nước: cho đi lại
                }
            }
            case "OPP_MOVE" -> {                          // OPP_MOVE|uci|flag|fen
                board.setFen(msg.arg(2));
                board.setLastMove(msg.arg(0));
                updateTurn(msg.arg(2));
                status.setText(msg.arg(1).equals("CHECK") ? "Bạn đang bị chiếu!" : "");
            }
            case "GAME_OVER" -> showGameOver(msg);
            default -> { }
        }
    }

    // Phần 2 của FEN cho biết lượt ai: mở/khóa bàn cờ theo đó
    private void updateTurn(String fen) {
        String side = fen.split(" ")[1];                       // "w" hoặc "b"
        boolean myTurn = side.equals(myColor.equals("WHITE") ? "w" : "b");
        board.setEnabled(myTurn);
        turnLabel.setText(myTurn ? "▶ Lượt của bạn" : "Đang chờ đối thủ...");
    }

    // GAME_OVER|result|reason|new_rating|delta
    private void showGameOver(Message msg) {
        board.setEnabled(false);
        String result = switch (msg.arg(0)) {
            case "WIN"  -> "Bạn thắng!";
            case "LOSS" -> "Bạn thua.";
            default     -> "Hòa.";
        };
        app.myRating = Integer.parseInt(msg.arg(2));

        Alert alert = new Alert(Alert.AlertType.INFORMATION);
        alert.setTitle("Kết thúc ván");
        alert.setHeaderText(result);
        alert.setContentText("Lý do: " + msg.arg(1) + "\nRating mới: " + msg.arg(2) + " (" + msg.arg(3) + ")");
        alert.showAndWait();
        app.show(new LobbyView(app));
    }
}