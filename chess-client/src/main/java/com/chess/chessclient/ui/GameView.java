package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.Alert;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.layout.VBox;

public class GameView implements Screen {
    private final ChessApp app;
    private final VBox root = new VBox(12);

    private final String roomId;
    private final String myColor;      // "WHITE" hoặc "BLACK"
    private final String opp;
    private final String oppRating;
    private String fen;

    private final Label fenLabel = new Label();

    // msg = GAME_START|room_id|source|color|opp|opp_rating|fen
    public GameView(ChessApp app, Message msg) {
        this.app = app;
        this.roomId    = msg.arg(0);
        String source  = msg.arg(1);
        this.myColor   = msg.arg(2);
        this.opp       = msg.arg(3);
        this.oppRating = msg.arg(4);
        this.fen       = msg.arg(5);

        Label title = new Label("Ván " + roomId + " (" + source + ")");
        title.setStyle("-fx-font-size: 20px; -fx-font-weight: bold;");
        Label vs = new Label(app.myUser + " (" + (myColor.equals("WHITE") ? "Trắng" : "Đen") + ")"
                + "  vs  " + opp + " (" + oppRating + ")");
        fenLabel.setText(fen);

        Button resignBtn = new Button("Xin thua");
        resignBtn.setOnAction(e -> app.send("RESIGN"));

        root.getChildren().addAll(title, vs, new Label("Thế cờ (FEN):"), fenLabel, resignBtn);
        root.setAlignment(Pos.CENTER);
        root.setPadding(new Insets(30));
    }

    @Override
    public Parent root() { return root; }

    @Override
    public void onMessage(Message msg) {
        switch (msg.command()) {
            case "OPP_MOVE" -> {                         // OPP_MOVE|uci|flag|fen
                fen = msg.arg(2);
                fenLabel.setText(fen);
            }
            case "GAME_OVER" -> showGameOver(msg);
            default -> { }
        }
    }

    // GAME_OVER|result|reason|new_rating|delta
    private void showGameOver(Message msg) {
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
        alert.showAndWait();                              // chờ người dùng bấm OK

        app.show(new LobbyView(app));                     // server đã đưa mình về IDLE
    }
}