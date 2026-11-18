package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import com.chess.chessclient.net.Protocol;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.PasswordField;
import javafx.scene.control.RadioButton;
import javafx.scene.control.TextField;
import javafx.scene.control.ToggleGroup;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Region;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;

/**
 * Màn hình kết nối + đăng nhập / đăng ký (UC1, UC2).
 *
 *   REGISTER|user|pass         → 200|REGISTER|user | 409 USERNAME_TAKEN | 400 BAD_FORMAT
 *   LOGIN|user|pass|mode       → 200|LOGIN|user|rating|mode | 403 | 409 | 400
 *
 * Chế độ (RANDOM/RANKED) chỉ quyết định hàng đợi khi bấm Tìm trận; phòng riêng dùng được với cả hai.
 */
public class LoginView implements Screen {
    private final ChessApp app;
    private final StackPane root = new StackPane();
    private final TextField hostField = new TextField("localhost");
    private final TextField portField = new TextField(String.valueOf(Protocol.DEFAULT_PORT));
    private final TextField userField = new TextField();
    private final PasswordField passField = new PasswordField();
    private final RadioButton randomBtn = new RadioButton("Ngẫu nhiên");
    private final RadioButton rankedBtn = new RadioButton("Theo trình độ (rating gần nhau)");
    private final Button loginBtn = new Button("Đăng nhập");
    private final Button registerBtn = new Button("Đăng ký");
    private final Label message = new Label();
    private final Label connInfo = new Label();

    public LoginView(ChessApp app) {
        this.app = app;

        Label title = new Label("Cờ vua trực tuyến");
        title.setStyle("-fx-font-size: 26px; -fx-font-weight: bold;");

        ToggleGroup modes = new ToggleGroup();
        randomBtn.setToggleGroup(modes);
        rankedBtn.setToggleGroup(modes);
        rankedBtn.setSelected(true);

        GridPane form = new GridPane();
        form.setHgap(10);
        form.setVgap(10);
        int r = 0;
        form.addRow(r++, new Label("Server"), hostField);
        form.addRow(r++, new Label("Cổng"), portField);
        form.addRow(r++, new Label("Tên đăng nhập"), userField);
        form.addRow(r++, new Label("Mật khẩu"), passField);
        form.addRow(r++, new Label("Chế độ tìm trận"), new VBox(6, randomBtn, rankedBtn));
        hostField.setPrefColumnCount(18);
        userField.setPromptText("3–20 ký tự: chữ, số, _");
        passField.setPromptText("4–32 ký tự, không dấu cách, không |");

        loginBtn.setDefaultButton(true);
        loginBtn.setOnAction(e -> submit(true));
        registerBtn.setOnAction(e -> submit(false));
        HBox buttons = new HBox(10, loginBtn, registerBtn);

        message.setWrapText(true);
        message.setMaxWidth(380);
        connInfo.setStyle("-fx-text-fill: #666;");

        VBox box = new VBox(16, title, form, buttons, message, connInfo);
        box.setPadding(new Insets(30));
        box.setMaxWidth(440);
        box.setStyle("-fx-background-color: white; -fx-background-radius: 8; "
                + "-fx-effect: dropshadow(gaussian, rgba(0,0,0,0.15), 12, 0, 0, 2);");
        box.setAlignment(Pos.TOP_LEFT);
        root.getChildren().add(box);
        root.setStyle("-fx-background-color: #eef1f5;");
        StackPane.setAlignment(box, Pos.CENTER);
        StackPane.setMargin(box, new Insets(40));
        box.setMaxHeight(Region.USE_PREF_SIZE);

        refreshConnInfo();
    }

    @Override
    public Parent root() {
        return root;
    }

    public void showError(String text) {
        message.setStyle("-fx-text-fill: #c0392b;");
        message.setText(text);
        setBusy(false);
    }

    private void showInfo(String text) {
        message.setStyle("-fx-text-fill: #1e7e34;");
        message.setText(text);
    }

    private void refreshConnInfo() {
        boolean on = app.isConnected();
        connInfo.setText(on ? "Đã kết nối tới " + app.serverAddress() : "Chưa kết nối");
        hostField.setDisable(on);
        portField.setDisable(on);
    }

    private void setBusy(boolean busy) {
        loginBtn.setDisable(busy);
        registerBtn.setDisable(busy);
    }

    /** Kiểm tra định dạng ở client để báo lỗi sớm (server vẫn kiểm tra lại). */
    private void submit(boolean login) {
        String user = userField.getText().trim();
        String pass = passField.getText();
        if (!Protocol.USER.matcher(user).matches()) {
            showError("Tên đăng nhập: 3–20 ký tự, chỉ gồm chữ, số và dấu _");
            return;
        }
        if (!Protocol.PASS.matcher(pass).matches()) {
            showError("Mật khẩu: 4–32 ký tự, không có dấu cách và dấu |");
            return;
        }
        String mode = randomBtn.isSelected() ? Protocol.MODE_RANDOM : Protocol.MODE_RANKED;
        Runnable sendIt = () -> {
            if (login) app.send("LOGIN", user, pass, mode);
            else       app.send("REGISTER", user, pass);
        };

        message.setText("");
        setBusy(true);
        if (app.isConnected()) {
            sendIt.run();
            return;
        }
        int port;
        try {
            port = Integer.parseInt(portField.getText().trim());
        } catch (NumberFormatException e) {
            showError("Cổng phải là số");
            return;
        }
        String host = hostField.getText().trim();
        showInfo("Đang kết nối tới " + host + ":" + port + "...");
        app.connectAsync(host, port,
                () -> { refreshConnInfo(); sendIt.run(); },
                err -> showError("Không kết nối được tới server (" + err + ")"));
    }

    @Override
    public void onMessage(Message m) {
        if (!m.isResponse()) return;
        switch (m.name()) {
            case "LOGIN" -> {
                if (m.isOk()) {
                    // 200|LOGIN|user|rating|mode
                    app.onLoggedIn(m.arg(0), m.argInt(1, 0), m.arg(2));
                } else {
                    showError(Texts.error(m));
                }
            }
            case "REGISTER" -> {
                setBusy(false);
                if (m.isOk()) showInfo("Đăng ký thành công tài khoản " + m.arg(0) + ". Bây giờ hãy đăng nhập.");
                else          showError(Texts.error(m));
            }
            default -> {
                if (!m.isOk()) showError(Texts.error(m));
            }
        }
    }
}
