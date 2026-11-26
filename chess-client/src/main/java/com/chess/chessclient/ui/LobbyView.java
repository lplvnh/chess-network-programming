package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import javafx.beans.property.SimpleStringProperty;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.scene.control.ButtonBar;
import javafx.stage.Modality;
import javafx.animation.Animation;
import javafx.animation.KeyFrame;
import javafx.animation.Timeline;
import javafx.util.Duration;
import java.util.HashMap;
import java.util.Map;

public class LobbyView implements Screen {
    private final ChessApp app;
    private final BorderPane root = new BorderPane();

    private final TableView<String[]> onlineTable = new TableView<>();
    private final TableView<String[]> rankTable = new TableView<>();
    private final Label myRankLabel = new Label();
    private final Label header = new Label();
    private final Label status = new Label();
    private final Map<String, Alert> inviteDialogs = new HashMap<>();   // mã phòng → hộp thoại

    private final Button findBtn = new Button("Tìm trận");
    private boolean searching = false;
    private Timeline waitTimer;        // cập nhật "đã chờ n giây" mỗi giây khi đang tìm trận
    private long searchStart;          // thời điểm nhận 202 FIND_MATCH (mili giây)

    public LobbyView(ChessApp app) {
        this.app = app;

        // ----- Thanh trên: tên, rating, đăng xuất -----
        header.setStyle("-fx-font-size: 18px; -fx-font-weight: bold;");
        updateHeader();
        Button logoutBtn = new Button("Đăng xuất");
        logoutBtn.setOnAction(e -> app.send("LOGOUT"));
        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);              // đẩy nút sang phải
        HBox top = new HBox(10, header, spacer, logoutBtn);
        top.setAlignment(Pos.CENTER_LEFT);
        top.setPadding(new Insets(10));

        // ----- Bảng người online: dòng USER|name|rating|mode|status -----
        onlineTable.getColumns().add(column("Tên", 1));
        onlineTable.getColumns().add(column("Rating", 2));
        onlineTable.getColumns().add(column("Chế độ", 3));
        TableColumn<String[], String> statusCol = new TableColumn<>("Trạng thái");
        statusCol.setCellValueFactory(d -> new SimpleStringProperty(statusText(d.getValue()[4])));
        onlineTable.getColumns().add(statusCol);
        onlineTable.setColumnResizePolicy(TableView.CONSTRAINED_RESIZE_POLICY_FLEX_LAST_COLUMN);

        Button refreshBtn = new Button("Làm mới");
        refreshBtn.setOnAction(e -> refresh());
        VBox left = new VBox(8, new HBox(10, new Label("Người đang online"), refreshBtn), onlineTable);
        VBox.setVgrow(onlineTable, Priority.ALWAYS);

        // ----- Bảng xếp hạng: dòng RANK|rank|name|rating|W|D|L -----
        rankTable.getColumns().add(column("#", 1));
        rankTable.getColumns().add(column("Tên", 2));
        rankTable.getColumns().add(column("Rating", 3));
        rankTable.getColumns().add(column("Thắng", 4));
        rankTable.getColumns().add(column("Hòa", 5));
        rankTable.getColumns().add(column("Thua", 6));
        rankTable.setColumnResizePolicy(TableView.CONSTRAINED_RESIZE_POLICY_FLEX_LAST_COLUMN);
        VBox right = new VBox(8, new Label("Bảng xếp hạng"), rankTable, myRankLabel);
        VBox.setVgrow(rankTable, Priority.ALWAYS);

        HBox center = new HBox(16, left, right);
        HBox.setHgrow(left, Priority.ALWAYS);
        HBox.setHgrow(right, Priority.ALWAYS);
        center.setPadding(new Insets(10));

        // ----- Thanh dưới: nút chức năng (C5, C8 sẽ gắn việc) -----

        Button roomBtn = new Button("Tạo phòng");
        findBtn.setOnAction(e -> {
            closeAllInvites();
            if (!searching) app.send("FIND_MATCH");
            else            app.send("CANCEL_MATCH");
            findBtn.setDisable(true);            // khóa tới khi server trả lời
        });
        roomBtn.setOnAction(e -> {
            closeAllInvites();
            app.send("CREATE_ROOM");
        });
        HBox bottom = new HBox(12, findBtn, roomBtn, status);
        bottom.setAlignment(Pos.CENTER);
        bottom.setPadding(new Insets(12));

        root.setTop(top);
        root.setCenter(center);
        root.setBottom(bottom);

        refresh();     // vừa vào sảnh thì tải dữ liệu ngay
    }

    // Hiện hộp thoại mời (KHÔNG chặn cửa sổ, có thể hiện nhiều cái cùng lúc)
    private void showInvite(Message msg) {
        String host = msg.arg(0), rating = msg.arg(1), roomId = msg.arg(2);
        ButtonType accept  = new ButtonType("Đồng ý", ButtonBar.ButtonData.YES);
        ButtonType decline = new ButtonType("Từ chối", ButtonBar.ButtonData.NO);

        Alert dlg = new Alert(Alert.AlertType.CONFIRMATION,
                host + " (" + rating + ") mời bạn chơi một ván.", accept, decline);
        dlg.setTitle("Lời mời – phòng " + roomId);
        dlg.setHeaderText(null);
        dlg.initModality(Modality.NONE);          // không chặn cửa sổ chính

        dlg.setOnHidden(e -> {
            // Nếu không còn trong map → đã bị chương trình đóng (INVITE_CANCELLED…) → không gửi gì
            if (inviteDialogs.remove(roomId) == null) return;
            if (dlg.getResult() == accept) app.send("ACCEPT_INVITE|" + roomId);
            else                           app.send("DECLINE_INVITE|" + roomId);
        });

        inviteDialogs.put(roomId, dlg);
        dlg.show();                               // show(), KHÔNG phải showAndWait()
    }

    // Đóng mọi hộp thoại mời mà không gửi gì (server tự hủy lời mời khi mình rời IDLE)
    private void closeAllInvites() {
        for (Alert dlg : inviteDialogs.values().toArray(new Alert[0])) {
            inviteDialogs.clear();
            dlg.close();
        }
        inviteDialogs.clear();
    }

    @Override
    public Parent root() { return root; }

    private void refresh() {
        app.send("LIST_ONLINE");
        app.send("LEADERBOARD|10");
    }

    private void setSearching(boolean on) {
        searching = on;
        findBtn.setText(on ? "Hủy tìm trận" : "Tìm trận");
        if (waitTimer != null) { waitTimer.stop(); waitTimer = null; }
        if (!on) { status.setText(""); return; }
        // Hiện thời gian đã chờ (design UC7): với RANKED, chờ càng lâu khoảng rating càng nới
        searchStart = System.currentTimeMillis();
        updateSearchStatus();
        waitTimer = new Timeline(new KeyFrame(Duration.seconds(1), e -> updateSearchStatus()));
        waitTimer.setCycleCount(Animation.INDEFINITE);
        waitTimer.play();
    }

    private void updateSearchStatus() {
        if (!app.isShowing(this)) {        // đã rời sảnh (mất kết nối...) → dừng đồng hồ
            if (waitTimer != null) waitTimer.stop();
            return;
        }
        long seconds = (System.currentTimeMillis() - searchStart) / 1000;
        String what = "RANKED".equals(app.myMode) ? "Đang tìm đối thủ cùng trình độ" : "Đang tìm đối thủ";
        status.setText(what + " · đã chờ " + seconds + " giây");
    }

    @Override
    public void onMessage(Message msg) {
        switch (msg.command()) {
            case "LIST_ONLINE" -> {
                if (msg.isOk()) onlineTable.getItems().setAll(msg.data());
            }
            case "LEADERBOARD" -> {
                if (!msg.isOk()) return;
                rankTable.getItems().setAll(msg.data());
                // 200|LEADERBOARD|k|my_rank|my_rating
                app.myRating = Integer.parseInt(msg.arg(2));
                myRankLabel.setText("Hạng của bạn: #" + msg.arg(1) + " (" + msg.arg(2) + ")");
                updateHeader();
            }
            case "LOGOUT" -> {
                if (msg.isOk()) {
                    setSearching(false);
                    app.stopHeartbeat();
                    app.myUser = null;
                    app.show(new LoginView(app, ""));
                }
            }
            case "FIND_MATCH" -> {
                findBtn.setDisable(false);
                if (msg.isOk()) setSearching(true);              // 202
                else status.setText("Không tìm trận được: " + msg.arg(0));
            }
            case "CANCEL_MATCH" -> {
                findBtn.setDisable(false);
                if (msg.isOk()) setSearching(false);             // 200
                // 405 WRONG_STATE: vừa được ghép xong, GAME_START sắp tới → bỏ qua
            }
            case "CREATE_ROOM" -> {
                if (msg.isOk()) app.show(new RoomView(app, msg.arg(0)));    // 201|CREATE_ROOM|R3
                else status.setText("Không tạo được phòng: " + msg.arg(0));
            }
            case "INVITED" -> showInvite(msg);                              // INVITED|host|rating|room
            case "INVITE_CANCELLED" -> {                                    // INVITE_CANCELLED|room
                Alert dlg = inviteDialogs.remove(msg.arg(0));               // XÓA khỏi map TRƯỚC...
                if (dlg != null) dlg.close();                               // ...rồi mới đóng
            }
            case "ACCEPT_INVITE" -> {
                if (!msg.isOk()) status.setText(switch (msg.arg(0)) {
                    case "NO_SUCH_INVITE" -> "Lời mời không còn hiệu lực";
                    case "ROOM_FULL"      -> "Phòng đã đủ người";
                    default               -> "Lỗi: " + msg.arg(0);
                });
                // 200: chờ GAME_START
            }
            case "GAME_START" -> {
                setSearching(false);
                closeAllInvites();
                app.show(new GameView(app, msg));
            }
            default -> { }   // PING và các bản tin khác: bỏ qua
        }
    }

    private void updateHeader() {
        header.setText("♚ " + app.myUser + " · " + app.myRating + " · " + app.myMode);
    }

    // Tạo một cột hiển thị phần tử thứ 'index' của mỗi dòng String[]
    private static TableColumn<String[], String> column(String title, int index) {
        TableColumn<String[], String> col = new TableColumn<>(title);
        col.setCellValueFactory(d -> new SimpleStringProperty(d.getValue()[index]));
        return col;
    }

    private static String statusText(String s) {
        return switch (s) {
            case "IDLE"   -> "Rảnh";
            case "QUEUED" -> "Đang tìm trận";
            case "ROOM"   -> "Trong phòng";
            case "INGAME" -> "Đang chơi";
            default       -> s;
        };
    }
}