package com.chess.chessclient;

import com.chess.chessclient.net.Connection;
import com.chess.chessclient.net.Message;
import com.chess.chessclient.net.Protocol;
import com.chess.chessclient.ui.GameView;
import com.chess.chessclient.ui.InviteManager;
import com.chess.chessclient.ui.LobbyView;
import com.chess.chessclient.ui.LoginView;
import com.chess.chessclient.ui.Screen;
import javafx.animation.KeyFrame;
import javafx.animation.Timeline;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.stage.Stage;
import javafx.util.Duration;

import java.io.IOException;
import java.util.function.Consumer;

/**
 * Điểm vào của client và là "bộ điều phối" chung.
 *
 * Luồng chạy:
 *   luồng nhận (Connection) ──Platform.runLater──▶ dispatch() trên luồng JavaFX
 *   dispatch() tự xử lý vài bản tin chung (PING, lời mời, GAME_START),
 *   phần còn lại chuyển cho màn hình đang hiện (Screen.onMessage).
 */
public class ChessApp extends Application {
    private Stage stage;
    private Scene scene;
    private Connection conn;
    private Screen current;
    private Timeline heartbeat;
    private final InviteManager invites = new InviteManager(this);

    // Phiên đăng nhập hiện tại
    private String user = "";
    private int rating;
    private String mode = "";
    private String serverAddress = "";

    public static void main(String[] args) {
        launch(args);
    }

    @Override
    public void start(Stage stage) {
        this.stage = stage;
        stage.setTitle("Cờ vua trực tuyến");
        stage.setMinWidth(900);
        stage.setMinHeight(640);
        show(new LoginView(this));
        stage.show();
    }

    @Override
    public void stop() {
        stopHeartbeat();
        if (conn != null) conn.close();       // server thấy recv()=0 và tự dọn
        conn = null;
    }

    // ================= Kết nối =================

    public boolean isConnected() {
        return conn != null && !conn.isClosed();
    }

    public String serverAddress() {
        return serverAddress;
    }

    /** Kết nối ở luồng phụ để giao diện không bị đứng khi server không trả lời. */
    public void connectAsync(String host, int port, Runnable onOk, Consumer<String> onFail) {
        Thread t = new Thread(() -> {
            try {
                Connection c = Connection.open(host, port, 5000);
                Platform.runLater(() -> {
                    conn = c;
                    serverAddress = host + ":" + port;
                    c.startReceiver(
                            m -> Platform.runLater(() -> dispatch(m)),
                            () -> Platform.runLater(() -> onDisconnected(c)));
                    onOk.run();
                });
            } catch (IOException e) {
                Platform.runLater(() -> onFail.accept(e.getMessage()));
            }
        }, "connect");
        t.setDaemon(true);
        t.start();
    }

    /** Gửi một request. Lỗi ghi socket được coi như mất kết nối. */
    public void send(String line) {
        Connection c = conn;
        if (c == null || c.isClosed()) return;
        try {
            c.send(line);
        } catch (IOException e) {
            c.close();
            onDisconnected(c);
        }
    }

    /** Gửi CMD|f1|f2... (kiểm tra trường không chứa '|'). */
    public void send(String command, String... fields) {
        send(Protocol.line(command, fields));
    }

    private void onDisconnected(Connection c) {
        if (c != conn) return;                // đã xử lý rồi
        conn = null;
        stopHeartbeat();
        invites.closeAll();
        user = "";
        LoginView v = new LoginView(this);
        show(v);
        v.showError("Mất kết nối tới server");
    }

    // ================= Phiên đăng nhập =================

    public void onLoggedIn(String user, int rating, String mode) {
        this.user = user;
        this.rating = rating;
        this.mode = mode;
        startHeartbeat();
        show(new LobbyView(this));
    }

    public void onLoggedOut() {
        stopHeartbeat();
        invites.closeAll();
        user = "";
        show(new LoginView(this));
    }

    public String user()            { return user; }
    public int rating()             { return rating; }
    public void setRating(int r)    { rating = r; }
    public String mode()            { return mode; }
    public Stage stage()            { return stage; }
    public InviteManager invites()  { return invites; }

    /** Đã đăng nhập thì gửi PING mỗi 15 giây, nếu không server ngắt sau 45 giây im lặng. */
    private void startHeartbeat() {
        stopHeartbeat();
        heartbeat = new Timeline(new KeyFrame(Duration.seconds(Protocol.HEARTBEAT_SECONDS), e -> send("PING")));
        heartbeat.setCycleCount(Timeline.INDEFINITE);
        heartbeat.play();
    }

    private void stopHeartbeat() {
        if (heartbeat != null) heartbeat.stop();
        heartbeat = null;
    }

    // ================= Màn hình =================

    public void show(Screen s) {
        if (current != null) current.onLeave();
        current = s;
        if (scene == null) {
            scene = new Scene(s.root(), 1000, 700);
            stage.setScene(scene);
        } else {
            scene.setRoot(s.root());
        }
    }

    // ================= Điều phối bản tin =================

    private void dispatch(Message m) {
        // 1. Trả lời PING: không cần làm gì
        if (m.isResponse() && m.name().equals("PING")) return;

        // 2. Lời mời: có thể tới khi đang ở sảnh hoặc ở màn hình ván vừa kết thúc → xử lý chung
        if (m.isEvent() && m.name().equals("INVITED"))          { invites.onInvited(m); return; }
        if (m.isEvent() && m.name().equals("INVITE_CANCELLED")) { invites.onCancelled(m); return; }
        if (m.isResponse() && m.name().equals("ACCEPT_INVITE")) { invites.onAcceptResponse(m); return; }
        if (m.isResponse() && m.name().equals("DECLINE_INVITE")) return;

        // 3. Ván bắt đầu (từ ghép ngẫu nhiên, ghép theo trình độ hay phòng) → luôn mở màn hình ván
        if (m.isEvent() && m.name().equals("GAME_START")) {
            invites.closeAll();
            show(new GameView(this, m));
            return;
        }

        // 4. Còn lại: màn hình hiện tại tự xử lý
        if (current != null) current.onMessage(m);
    }
}
