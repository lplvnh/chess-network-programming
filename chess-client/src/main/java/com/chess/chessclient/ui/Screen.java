package com.chess.chessclient.ui;

import com.chess.chessclient.net.Message;
import javafx.scene.Parent;

// Một màn hình của ứng dụng
public interface Screen {
    Parent root();                 // giao diện để hiển thị
    void onMessage(Message msg);   // xử lý bản tin khi đang ở màn hình này
}