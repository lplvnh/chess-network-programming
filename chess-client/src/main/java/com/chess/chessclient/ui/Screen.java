package com.chess.chessclient.ui;

import com.chess.chessclient.net.Message;
import javafx.scene.Parent;

/** Một màn hình của client. ChessApp chuyển cho màn hình đang hiện những bản tin nó không tự xử lý. */
public interface Screen {
    Parent root();

    /** Chạy trên luồng JavaFX. */
    void onMessage(Message m);

    /** Gọi khi màn hình bị thay bằng màn hình khác (dừng timer...). */
    default void onLeave() { }
}
