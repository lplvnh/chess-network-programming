package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import javafx.beans.property.SimpleStringProperty;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.*;
import javafx.scene.layout.*;

public class LobbyView implements Screen {
    private final ChessApp app;
    private final BorderPane root = new BorderPane();

    private final TableView<String[]> onlineTable = new TableView<>();
    private final TableView<String[]> rankTable = new TableView<>();
    private final Label myRankLabel = new Label();
    private final Label header = new Label();
    private final Label status = new Label();

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
        Button findBtn = new Button("Tìm trận");
        Button roomBtn = new Button("Tạo phòng");
        findBtn.setDisable(true);   // C5
        roomBtn.setDisable(true);   // C8
        HBox bottom = new HBox(12, findBtn, roomBtn, status);
        bottom.setAlignment(Pos.CENTER);
        bottom.setPadding(new Insets(12));

        root.setTop(top);
        root.setCenter(center);
        root.setBottom(bottom);

        refresh();     // vừa vào sảnh thì tải dữ liệu ngay
    }

    @Override
    public Parent root() { return root; }

    private void refresh() {
        app.send("LIST_ONLINE");
        app.send("LEADERBOARD|10");
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
                    app.stopHeartbeat();
                    app.myUser = null;
                    app.show(new LoginView(app, ""));
                }
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