package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ListView;
import javafx.scene.control.TableView;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.Region;
import javafx.scene.layout.VBox;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Phòng chờ của chủ phòng (ROOM_WAITING, UC8).
 *
 *   INVITE|user      → 200|INVITE|user | 400 SELF_INVITE | 404 USER_NOT_ONLINE | 409 USER_BUSY
 *   event INVITE_DECLINED|user   (từ chối, hết hạn 60 giây, bận việc khác hoặc mất kết nối)
 *   LEAVE_ROOM       → 200|LEAVE_ROOM → về sảnh
 *   Có người nhận lời → event GAME_START (ChessApp tự mở màn hình ván)
 */
public class RoomView implements Screen {
    private final ChessApp app;
    private final String roomId;
    private final BorderPane root = new BorderPane();
    private final TableView<String[]> onlineTable = Tables.create("Chưa có dữ liệu");
    private final ListView<String> invitedList = new ListView<>();
    private final Map<String, String> invited = new LinkedHashMap<>();   // tên → tình trạng
    private final Label status = new Label();

    public RoomView(ChessApp app, String roomId) {
        this.app = app;
        this.roomId = roomId;

        Label title = new Label("Phòng " + roomId + " — đang chờ đối thủ");
        title.setStyle("-fx-font-size: 18px; -fx-font-weight: bold;");
        Label me = new Label(app.user() + " (" + app.rating() + ")");
        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);
        Button leaveBtn = new Button("Rời phòng");
        leaveBtn.setOnAction(e -> app.send("LEAVE_ROOM"));
        HBox top = new HBox(12, title, me, spacer, leaveBtn);
        top.setAlignment(Pos.CENTER_LEFT);
        top.setPadding(new Insets(12));
        top.setStyle("-fx-background-color: #f4f6f8; -fx-border-color: transparent transparent #dde1e6 transparent;");

        Tables.column(onlineTable, "Tên", 1);
        Tables.column(onlineTable, "Rating", 2);
        Tables.column(onlineTable, "Chế độ", 3, Texts::mode);
        Tables.column(onlineTable, "Trạng thái", 4, Texts::status);

        Button refreshBtn = new Button("Làm mới");
        refreshBtn.setOnAction(e -> app.send("LIST_ONLINE"));
        Button inviteBtn = new Button("Mời người đã chọn");
        inviteBtn.setStyle("-fx-font-weight: bold;");
        inviteBtn.setOnAction(e -> inviteSelected());
        onlineTable.setRowFactory(tv -> {
            var row = new javafx.scene.control.TableRow<String[]>();
            row.setOnMouseClicked(ev -> { if (ev.getClickCount() == 2 && !row.isEmpty()) inviteSelected(); });
            return row;
        });

        Label leftTitle = new Label("Người đang online (chọn rồi bấm Mời, hoặc nhấp đúp)");
        VBox left = new VBox(8, new HBox(10, leftTitle, refreshBtn), onlineTable, inviteBtn);
        VBox.setVgrow(onlineTable, Priority.ALWAYS);
        VBox right = new VBox(8, new Label("Lời mời đã gửi"), invitedList);
        VBox.setVgrow(invitedList, Priority.ALWAYS);
        right.setPrefWidth(300);

        HBox center = new HBox(16, left, right);
        HBox.setHgrow(left, Priority.ALWAYS);
        center.setPadding(new Insets(12));

        status.setPadding(new Insets(0, 12, 12, 12));

        root.setTop(top);
        root.setCenter(center);
        root.setBottom(status);

        app.send("LIST_ONLINE");
    }

    @Override
    public Parent root() {
        return root;
    }

    private void inviteSelected() {
        String[] row = onlineTable.getSelectionModel().getSelectedItem();
        if (row == null || row.length < 2) { setStatus("Hãy chọn một người trong bảng", true); return; }
        String name = row[1];
        if (name.equals(app.user())) { setStatus("Không thể tự mời mình", true); return; }
        setStatus("", false);
        app.send("INVITE", name);
    }

    private void setStatus(String text, boolean error) {
        status.setStyle(error ? "-fx-text-fill: #c0392b;" : "-fx-text-fill: #1e7e34;");
        status.setText(text);
    }

    private void setInvited(String name, String state) {
        invited.put(name, state);
        invitedList.getItems().setAll(
                invited.entrySet().stream().map(e -> e.getKey() + " — " + e.getValue()).toList());
    }

    @Override
    public void onMessage(Message m) {
        if (m.isEvent()) {
            if (m.name().equals("INVITE_DECLINED")) {           // INVITE_DECLINED|user
                setInvited(m.arg(0), "từ chối / hết hạn / bận");
                setStatus(m.arg(0) + " không nhận lời. Bạn có thể mời người khác.", true);
            }
            return;
        }
        switch (m.name()) {
            case "LIST_ONLINE" -> {
                if (m.isOk()) onlineTable.getItems().setAll(m.data());
            }
            case "INVITE" -> {
                if (m.isOk()) {                                  // 200|INVITE|user
                    setInvited(m.arg(0), "đang chờ trả lời (60 giây)");
                    setStatus("Đã gửi lời mời tới " + m.arg(0), false);
                } else {
                    setStatus(Texts.error(m), true);
                }
            }
            case "LEAVE_ROOM" -> {
                if (m.isOk()) app.show(new LobbyView(app));
                else setStatus(Texts.error(m), true);
            }
            default -> {
                if (!m.isOk()) setStatus(Texts.error(m), true);
            }
        }
    }

    public String roomId() {
        return roomId;
    }
}
