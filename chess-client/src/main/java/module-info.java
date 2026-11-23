module com.chess.chessclient {
    requires javafx.controls;
    requires javafx.graphics;      // ← thêm dòng này cho chắc (nơi chứa Parent, Scene, Stage)

    // Giao diện dựng bằng code Java, không dùng FXML nên không cần javafx.fxml
    exports com.chess.chessclient;   // JavaFX cần truy cập ChessApp (lớp Application) để khởi động
}