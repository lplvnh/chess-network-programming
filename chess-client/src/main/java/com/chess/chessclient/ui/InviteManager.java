package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import javafx.scene.control.Alert;
import javafx.scene.control.ButtonBar;
import javafx.scene.control.ButtonType;
import javafx.stage.Modality;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * Hộp thoại lời mời của người được mời (UC9).
 *
 * - INVITED|host|rating|room  → hiện một hộp thoại KHÔNG chặn (có thể có nhiều lời mời cùng lúc)
 * - Người dùng bấm Đồng ý / Từ chối → ACCEPT_INVITE|room / DECLINE_INVITE|room
 * - INVITE_CANCELLED|room     → đóng đúng hộp thoại đó, KHÔNG gửi gì
 * - Mình rời trạng thái rảnh (tìm trận, tạo phòng, vào ván, đăng xuất) → đóng hết, KHÔNG gửi gì
 *   (server tự hủy lời mời và báo cho chủ phòng)
 */
public final class InviteManager {
    private final ChessApp app;
    private final Map<String, Alert> open = new HashMap<>();     // mã phòng → hộp thoại

    public InviteManager(ChessApp app) {
        this.app = app;
    }

    public void onInvited(Message m) {
        String host = m.arg(0), rating = m.arg(1), room = m.arg(2);
        closeSilently(room);                                     // mời lại cùng phòng: thay hộp thoại cũ

        ButtonType accept = new ButtonType("Đồng ý", ButtonBar.ButtonData.YES);
        ButtonType decline = new ButtonType("Từ chối", ButtonBar.ButtonData.CANCEL_CLOSE);
        Alert dlg = new Alert(Alert.AlertType.CONFIRMATION,
                host + " (" + rating + ") mời bạn chơi một ván.", accept, decline);
        dlg.setTitle("Lời mời – phòng " + room);
        dlg.setHeaderText(null);
        dlg.initOwner(app.stage());
        dlg.initModality(Modality.NONE);                          // không chặn cửa sổ chính

        dlg.setOnHidden(e -> {
            // Đã bị chương trình xóa khỏi map (INVITE_CANCELLED, closeAll) → không gửi gì
            if (open.get(room) != dlg) return;
            open.remove(room);
            if (dlg.getResult() == accept) app.send("ACCEPT_INVITE", room);
            else                           app.send("DECLINE_INVITE", room);
        });
        open.put(room, dlg);
        dlg.show();                                               // show(), không phải showAndWait()
    }

    public void onCancelled(Message m) {
        closeSilently(m.arg(0));
    }

    /** 200|ACCEPT_INVITE: chờ GAME_START. Lỗi 404/410: báo cho người dùng. */
    public void onAcceptResponse(Message m) {
        if (m.isOk()) return;
        Alert a = new Alert(Alert.AlertType.INFORMATION, Texts.error(m));
        a.setHeaderText(null);
        a.setTitle("Lời mời");
        a.initOwner(app.stage());
        a.initModality(Modality.NONE);
        a.show();
    }

    public void closeAll() {
        for (String room : new ArrayList<>(open.keySet())) closeSilently(room);
    }

    private void closeSilently(String room) {
        Alert dlg = open.remove(room);                            // xóa khỏi map TRƯỚC rồi mới đóng
        if (dlg != null) dlg.close();
    }
}
