module com.chess.chessclient {
    requires javafx.controls;
    requires javafx.fxml;


    opens com.chess.chessclient to javafx.fxml;
    exports com.chess.chessclient;
}