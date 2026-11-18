module com.chess.chessclient {
    requires javafx.controls;
    requires javafx.graphics;      // ← thêm dòng này cho chắc (nơi chứa Parent, Scene, Stage)
    requires javafx.fxml;

    opens com.chess.chessclient to javafx.fxml;
    exports com.chess.chessclient;
}