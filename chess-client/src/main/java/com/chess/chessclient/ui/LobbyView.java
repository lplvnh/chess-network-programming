package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import javafx.animation.KeyFrame;
import javafx.animation.Timeline;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TableView;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;
import javafx.util.Duration;

/**
 * Sảnh (IDLE / QUEUED).
 *
 *   UC4  LIST_ONLINE            → 200|LIST_ONLINE|n + n dòng USER|user|rating|mode|status
 *   UC5  LEADERBOARD|10         → 200|LEADERBOARD|k|my_rank|my_rating + k dòng RANK|rank|user|rating|W|D|L
 *   UC6/7 FIND_MATCH            → 202|FIND_MATCH|mode, sau đó event GAME_START (ChessApp mở màn hình ván)
 *         CANCEL_MATCH          → 200 | 405 (vừa được ghép xong: bỏ qua, GAME_START sắp tới)
 *   UC8  CREATE_ROOM            → 201|CREATE_ROOM|room → màn hình phòng chờ
 *   UC3  LOGOUT                 → 200|LOGOUT → màn hình đăng nhập
 */
public class LobbyView implements Screen {
    private static final int LEADERBOARD_SIZE = 10;

    private final ChessApp app;
    private final BorderPane root = new BorderPane();
    private final Label hello = new Label();
    private final TableView<String[]> onlineTable = Tables.create("Chưa có dữ liệu");
    private final TableView<String[]> rankTable = Tables.create("Chưa có dữ liệu");
    private final Label myRank = new Label();
    private final Button findBtn = new Button("Tìm trận");
    private final Button cancelBtn = new Button("Hủy tìm");
    private final Button roomBtn = new Button("Tạo phòng");
    private final Button logoutBtn = new Button("Đăng xuất");
    private final Label status = new Label();

    private Timeline searchTimer;
    private int searchSeconds;

    public LobbyView(ChessApp app) {
        this.app = app;

        // ----- thanh trên -----
        hello.setStyle("-fx-font-size: 16px;");
        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);
        logoutBtn.setOnAction(e -> app.send("LOGOUT"));
        HBox top = new HBox(10, hello, spacer, logoutBtn);
        top.setAlignment(Pos.CENTER_LEFT);
        top.setPadding(new Insets(12));
        top.setStyle("-fx-background-color: #f4f6f8; -fx-border-color: transparent transparent #dde1e6 transparent;");
        updateHello();

        // ----- bảng người online -----
        Tables.column(onlineTable, "Tên", 1);
        Tables.column(onlineTable, "Rating", 2);
        Tables.column(onlineTable, "Chế độ", 3, Texts::mode);
        Tables.column(onlineTable, "Trạng thái", 4, Texts::status);
        Button refreshOnline = new Button("Làm mới");
        refreshOnline.setOnAction(e -> app.send("LIST_ONLINE"));
        VBox onlineBox = section("Người chơi đang online", refreshOnline, onlineTable, new Label(""));

        // ----- bảng xếp hạng -----
        Tables.column(rankTable, "Hạng", 1);
        Tables.column(rankTable, "Tên", 2);
        Tables.column(rankTable, "Rating", 3);
        Tables.column(rankTable, "Thắng", 4);
        Tables.column(rankTable, "Hòa", 5);
        Tables.column(rankTable, "Thua", 6);
        Button refreshRank = new Button("Làm mới");
        refreshRank.setOnAction(e -> app.send("LEADERBOARD", String.valueOf(LEADERBOARD_SIZE)));
        VBox rankBox = section("Bảng xếp hạng", refreshRank, rankTable, myRank);

        HBox center = new HBox(16, onlineBox, rankBox);
        HBox.setHgrow(onlineBox, Priority.ALWAYS);
        HBox.setHgrow(rankBox, Priority.ALWAYS);
        center.setPadding(new Insets(12));

        // ----- thanh dưới -----
        findBtn.setStyle("-fx-font-weight: bold;");
        findBtn.setOnAction(e -> {
            app.invites().closeAll();             // rời trạng thái rảnh: server tự hủy lời mời
            status.setText("");
            app.send("FIND_MATCH");
        });
        cancelBtn.setOnAction(e -> app.send("CANCEL_MATCH"));
        roomBtn.setOnAction(e -> {
            app.invites().closeAll();
            status.setText("");
            app.send("CREATE_ROOM");
        });
        HBox bottom = new HBox(10, findBtn, cancelBtn, roomBtn, status);
        bottom.setAlignment(Pos.CENTER_LEFT);
        bottom.setPadding(new Insets(12));
        setSearching(false);

        root.setTop(top);
        root.setCenter(center);
        root.setBottom(bottom);

        app.send("LIST_ONLINE");
        app.send("LEADERBOARD", String.valueOf(LEADERBOARD_SIZE));
    }

    private static VBox section(String title, Button refresh, TableView<String[]> table, Label footer) {
        Label t = new Label(title);
        t.setStyle("-fx-font-size: 15px; -fx-font-weight: bold;");
        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);
        HBox head = new HBox(8, t, spacer, refresh);
        head.setAlignment(Pos.CENTER_LEFT);
        VBox box = new VBox(8, head, table, footer);
        VBox.setVgrow(table, Priority.ALWAYS);
        return box;
    }

    private void updateHello() {
        hello.setText("Xin chào " + app.user() + "  ·  Rating " + app.rating()
                + "  ·  Chế độ tìm trận: " + Texts.mode(app.mode()));
    }

    @Override
    public Parent root() {
        return root;
    }

    @Override
    public void onLeave() {
        stopTimer();
    }

    private void setSearching(boolean on) {
        findBtn.setDisable(on);
        roomBtn.setDisable(on);
        logoutBtn.setDisable(false);
        cancelBtn.setVisible(on);
        cancelBtn.setManaged(on);
        stopTimer();
        if (on) {
            searchSeconds = 0;
            showSearching();
            searchTimer = new Timeline(new KeyFrame(Duration.seconds(1), e -> { searchSeconds++; showSearching(); }));
            searchTimer.setCycleCount(Timeline.INDEFINITE);
            searchTimer.play();
        }
    }

    private void showSearching() {
        status.setStyle("-fx-text-fill: #1f4e79;");
        status.setText(String.format("Đang tìm đối thủ (%s)... %d:%02d",
                Texts.mode(app.mode()), searchSeconds / 60, searchSeconds % 60));
    }

    private void stopTimer() {
        if (searchTimer != null) searchTimer.stop();
        searchTimer = null;
    }

    private void showError(Message m) {
        status.setStyle("-fx-text-fill: #c0392b;");
        status.setText(Texts.error(m));
    }

    @Override
    public void onMessage(Message m) {
        if (!m.isResponse()) return;
        switch (m.name()) {
            case "LIST_ONLINE" -> {
                if (m.isOk()) onlineTable.getItems().setAll(m.data());
            }
            case "LEADERBOARD" -> {
                if (m.isOk()) {
                    rankTable.getItems().setAll(m.data());
                    // 200|LEADERBOARD|k|my_rank|my_rating
                    app.setRating(m.argInt(2, app.rating()));
                    myRank.setText("Hạng của bạn: #" + m.arg(1) + "  (rating " + m.arg(2) + ")");
                    updateHello();
                }
            }
            case "FIND_MATCH" -> {
                if (m.isOk()) setSearching(true);              // 202|FIND_MATCH|mode
                else showError(m);
            }
            case "CANCEL_MATCH" -> {
                if (m.isOk()) { setSearching(false); status.setText("Đã hủy tìm trận"); }
                // 405: vừa được ghép xong, GAME_START đang tới → bỏ qua
            }
            case "CREATE_ROOM" -> {
                if (m.isOk()) app.show(new RoomView(app, m.arg(0)));   // 201|CREATE_ROOM|R7
                else showError(m);
            }
            case "LOGOUT" -> {
                if (m.isOk()) app.onLoggedOut();
                else showError(m);
            }
            default -> {
                if (!m.isOk()) showError(m);
            }
        }
    }
}
