package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import javafx.beans.property.SimpleStringProperty;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.*;
import javafx.scene.layout.*;

import java.util.LinkedHashMap;
import java.util.Map;

public class RoomView implements Screen {
    private final ChessApp app;
    private final BorderPane root = new BorderPane();
    private final TableView<String[]> onlineTable = new TableView<>();
    private final ListView<String> invitedList = new ListView<>();
    private final Map<String, String> invited = new LinkedHashMap<>();   // tên → tình trạng
    private final Label status = new Label();

    public RoomView(ChessApp app, String roomId) {
        this.app = app;

        Label title = new Label("Phòng " + roomId + " — đang chờ đối thủ");
        title.setStyle("-fx-font-size: 18px; -fx-font-weight: bold;");
        Button leaveBtn = new Button("Rời phòng");
        leaveBtn.setOnAction(e -> app.send("LEAVE_ROOM"));
        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);
        HBox top = new HBox(10, title, spacer, leaveBtn);
        top.setAlignment(Pos.CENTER_LEFT);
        top.setPadding(new Insets(10));

        // Bảng online (dòng USER|name|rating|mode|status)
        onlineTable.getColumns().add(column("Tên", 1));
        onlineTable.getColumns().add(column("Rating", 2));
        onlineTable.getColumns().add(column("Chế độ", 3));
        onlineTable.getColumns().add(column("Trạng thái", 4));
        onlineTable.setColumnResizePolicy(TableView.CONSTRAINED_RESIZE_POLICY_FLEX_LAST_COLUMN);

        Button refreshBtn = new Button("Làm mới");
        refreshBtn.setOnAction(e -> app.send("LIST_ONLINE"));
        Button inviteBtn = new Button("Mời người đã chọn");
        inviteBtn.setOnAction(e -> inviteSelected());

        VBox left = new VBox(8, new HBox(10, new Label("Người đang online"), refreshBtn),
                onlineTable, inviteBtn);
        VBox.setVgrow(onlineTable, Priority.ALWAYS);
        VBox right = new VBox(8, new Label("Lời mời đã gửi"), invitedList);
        VBox.setVgrow(invitedList, Priority.ALWAYS);

        HBox center = new HBox(16, left, right);
        HBox.setHgrow(left, Priority.ALWAYS);
        HBox.setHgrow(right, Priority.ALWAYS);
        center.setPadding(new Insets(10));

        status.setStyle("-fx-text-fill: #c0392b;");
        status.setPadding(new Insets(10));
        root.setTop(top);
        root.setCenter(center);
        root.setBottom(status);

        app.send("LIST_ONLINE");
    }

    @Override
    public Parent root() { return root; }

    private void inviteSelected() {
        String[] row = onlineTable.getSelectionModel().getSelectedItem();
        if (row == null) { status.setText("Hãy chọn một người trong bảng"); return; }
        String name = row[1];
        if (name.equals(app.myUser)) { status.setText("Không thể tự mời mình"); return; }
        status.setText("");
        app.send("INVITE|" + name);
    }

    @Override
    public void onMessage(Message msg) {
        switch (msg.command()) {
            case "LIST_ONLINE" -> {
                if (msg.isOk()) onlineTable.getItems().setAll(msg.data());
            }
            case "INVITE" -> {
                if (msg.isOk()) setInvited(msg.arg(0), "đang chờ trả lời");     // 200|INVITE|bob
                else status.setText(switch (msg.arg(0)) {
                    case "SELF_INVITE"     -> "Không thể tự mời mình";
                    case "USER_NOT_ONLINE" -> "Người này không còn online";
                    case "USER_BUSY"       -> "Người này đang bận";
                    default                -> "Lỗi: " + msg.arg(0);
                });
            }
            case "INVITE_DECLINED" -> setInvited(msg.arg(0), "đã từ chối / hết hạn");
            case "LEAVE_ROOM" -> {
                if (msg.isOk()) app.show(new LobbyView(app));
            }
            case "GAME_START" -> app.show(new GameView(app, msg));     // có người nhận lời
            default -> { }
        }
    }

    private void setInvited(String name, String state) {
        invited.put(name, state);
        invitedList.getItems().setAll(
                invited.entrySet().stream().map(e -> e.getKey() + " — " + e.getValue()).toList());
    }

    private static TableColumn<String[], String> column(String title, int index) {
        TableColumn<String[], String> col = new TableColumn<>(title);
        col.setCellValueFactory(d -> new SimpleStringProperty(d.getValue()[index]));
        return col;
    }
}