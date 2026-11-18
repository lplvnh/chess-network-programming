package com.chess.chessclient;

import com.chess.chessclient.net.Connection;
import com.chess.chessclient.ui.LoginView;
import com.chess.chessclient.ui.Screen;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.stage.Stage;
import javafx.animation.KeyFrame;
import javafx.animation.Timeline;
import javafx.util.Duration;

import java.io.IOException;

public class ChessApp extends Application {
    private Stage stage;
    private final Connection conn = new Connection();
    private boolean connected = false;
    private Screen current;

    // Thông tin người đang đăng nhập (dùng ở các màn hình sau)
    public String myUser;
    public int myRating;
    public String myMode;
    private Timeline heartbeat;

    // Gửi PING mỗi 15 giây (bắt đầu sau khi đăng nhập)
    public void startHeartbeat() {
        stopHeartbeat();
        heartbeat = new Timeline(new KeyFrame(Duration.seconds(15), e -> send("PING")));
        heartbeat.setCycleCount(Timeline.INDEFINITE);      // lặp mãi
        heartbeat.play();
    }

    public void stopHeartbeat() {
        if (heartbeat != null) heartbeat.stop();
        heartbeat = null;
    }

    @Override
    public void start(Stage stage) {
        this.stage = stage;
        stage.setTitle("Online Chess");
        show(new LoginView(this, ""));
        stage.show();
    }

    // Chuyển sang màn hình khác
    public void show(Screen screen) {
        current = screen;
        if (stage.getScene() == null) stage.setScene(new Scene(screen.root(), 900, 650));
        else stage.getScene().setRoot(screen.root());
    }

    // Kết nối nếu chưa kết nối. Trả false nếu không tới được server
    public boolean ensureConnected(String host) {
        if (connected) return true;
        try {
            conn.connect(host, 5500);
        } catch (IOException e) {
            return false;
        }
        connected = true;
        conn.startReceiving(
                msg -> Platform.runLater(() -> current.onMessage(msg)),   // ← chuyển sang luồng JavaFX
                () -> Platform.runLater(this::onDisconnected));
        return true;
    }

    // Gửi một bản tin (mọi màn hình đều gửi qua đây)
    public void send(String line) {
        try {
            conn.send(line);
        } catch (IOException e) {
            onDisconnected();
        }
    }

    private void onDisconnected() {
        if (!connected) return;
        stopHeartbeat();
        connected = false;
        conn.close();
        myUser = null;
        show(new LoginView(this, "Mất kết nối tới server"));
    }

    @Override
    public void stop() {          // đóng cửa sổ → đóng socket
        conn.close();
    }

    public static void main(String[] args) {
        launch();
    }
}