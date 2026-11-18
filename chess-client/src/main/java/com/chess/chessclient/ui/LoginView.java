package com.chess.chessclient.ui;

import com.chess.chessclient.ChessApp;
import com.chess.chessclient.net.Message;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.*;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;

public class LoginView implements Screen {
    private final ChessApp app;
    private final VBox root = new VBox(14);

    private final TextField hostField = new TextField("localhost");
    private final TextField userField = new TextField();
    private final PasswordField passField = new PasswordField();
    private final RadioButton randomBtn = new RadioButton("Ngẫu nhiên (RANDOM)");
    private final RadioButton rankedBtn = new RadioButton("Theo trình độ (RANKED)");
    private final Button loginBtn = new Button("Đăng nhập");
    private final Button registerBtn = new Button("Đăng ký");
    private final Label status = new Label();

    public LoginView(ChessApp app, String initialStatus) {
        this.app = app;

        // Chọn chế độ ghép trận
        ToggleGroup modes = new ToggleGroup();
        randomBtn.setToggleGroup(modes);
        rankedBtn.setToggleGroup(modes);
        rankedBtn.setSelected(true);

        // Lưới: nhãn bên trái, ô nhập bên phải
        GridPane form = new GridPane();
        form.setHgap(10);
        form.setVgap(10);
        form.addRow(0, new Label("Server:"), hostField);
        form.addRow(1, new Label("Tên đăng nhập:"), userField);
        form.addRow(2, new Label("Mật khẩu:"), passField);
        form.addRow(3, new Label("Chế độ ghép:"), new VBox(6, randomBtn, rankedBtn));
        form.setAlignment(Pos.CENTER);

        HBox buttons = new HBox(10, loginBtn, registerBtn);
        buttons.setAlignment(Pos.CENTER);

        Label title = new Label("♚ Cờ vua trực tuyến");
        title.setStyle("-fx-font-size: 28px; -fx-font-weight: bold;");

        status.setText(initialStatus);
        status.setStyle("-fx-text-fill: #c0392b;");

        root.getChildren().addAll(title, form, buttons, status);
        root.setAlignment(Pos.CENTER);
        root.setPadding(new Insets(30));

        loginBtn.setOnAction(e -> submit("LOGIN"));
        registerBtn.setOnAction(e -> submit("REGISTER"));
        passField.setOnAction(e -> submit("LOGIN"));       // Enter trong ô mật khẩu = Đăng nhập
    }

    @Override
    public Parent root() { return root; }

    // Bấm Đăng nhập hoặc Đăng ký
    private void submit(String command) {
        String user = userField.getText().trim();
        String pass = passField.getText();

        // Kiểm tra trước ở client (cùng quy tắc với server)
        if (!user.matches("[A-Za-z0-9_]{3,20}")) {
            showError("Tên: 3–20 ký tự, chỉ gồm chữ, số, dấu _");
            return;
        }
        if (!pass.matches("[!-~]{4,32}")) {                 // ký tự in được, không dấu cách
            showError("Mật khẩu: 4–32 ký tự, không dấu cách");
            return;
        }
        if (pass.contains("|")) {
            showError("Mật khẩu không được chứa dấu |");
            return;
        }

        if (!app.ensureConnected(hostField.getText().trim())) {
            showError("Không kết nối được tới server");
            return;
        }

        setBusy(true);
        if (command.equals("REGISTER")) {
            app.send("REGISTER|" + user + "|" + pass);
        } else {
            String mode = randomBtn.isSelected() ? "RANDOM" : "RANKED";
            app.send("LOGIN|" + user + "|" + pass + "|" + mode);
        }
    }

    @Override
    public void onMessage(Message msg) {
        switch (msg.command()) {
            case "REGISTER" -> {
                setBusy(false);
                if (msg.isOk()) showInfo("Đăng ký thành công, hãy đăng nhập");
                else showError(errorText(msg));
            }
            case "LOGIN" -> {
                setBusy(false);
                if (msg.isOk()) {
                    app.myUser = msg.arg(0);
                    app.myRating = Integer.parseInt(msg.arg(1));
                    app.myMode = msg.arg(2);
                    app.startHeartbeat();
                    app.show(new LobbyView(app));
                } else {
                    showError(errorText(msg));
                }
            }
            default -> { }   // bản tin khác: màn hình này không quan tâm
        }
    }

    // Đổi mã lỗi của server sang câu tiếng Việt
    private String errorText(Message msg) {
        return switch (msg.arg(0)) {
            case "BAD_FORMAT"          -> "Dữ liệu không hợp lệ";
            case "USERNAME_TAKEN"      -> "Tên đăng nhập đã tồn tại";
            case "INVALID_CREDENTIALS" -> "Sai tên đăng nhập hoặc mật khẩu";
            case "ALREADY_ONLINE"      -> "Tài khoản đang đăng nhập ở nơi khác";
            case "WRONG_STATE"         -> "Bạn đã đăng nhập rồi";
            default                    -> "Lỗi " + msg.code() + ": " + msg.arg(0);
        };
    }

    private void setBusy(boolean busy) {
        loginBtn.setDisable(busy);
        registerBtn.setDisable(busy);
    }

    private void showError(String text) {
        status.setStyle("-fx-text-fill: #c0392b;");
        status.setText(text);
    }

    private void showInfo(String text) {
        status.setStyle("-fx-text-fill: #27ae60;");
        status.setText(text);
    }
}