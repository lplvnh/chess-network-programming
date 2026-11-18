# chess-client

Client JavaFX cho ứng dụng Cờ vua trực tuyến. Nói chuyện với `chess-server` qua TCP (cổng 5500)
theo đúng giao thức trong tài liệu thiết kế: mỗi bản tin một dòng, các trường cách nhau bởi `|`.

## Yêu cầu

- JDK 21
- Maven (IntelliJ có sẵn)

## Chạy

**IntelliJ:** File → Open → chọn thư mục `chess-client` (mở như Maven project) → chạy `ChessApp`.

**Dòng lệnh:**

```bash
cd chess-client
mvn javafx:run
```

Chạy nhiều cửa sổ để thử nhiều người chơi: mỗi lần chạy là một client.
Ở màn hình đăng nhập, ô **Server** là địa chỉ máy chạy server (`localhost` nếu cùng máy, IP LAN nếu khác máy).

**Client dòng lệnh (giống `nc`)** để thử tầng mạng không cần giao diện: chạy `ConsoleClient` (tham số: host, port),
rồi gõ bản tin, ví dụ `LOGIN|alice|secret12|RANKED`.

## Cấu trúc

```
src/main/java/com/chess/chessclient/
├── ChessApp.java        Điểm vào; giữ kết nối, phiên đăng nhập, PING 15 giây; điều phối bản tin tới màn hình
├── ConsoleClient.java   Client dòng lệnh để thử giao thức
├── net/
│   ├── Connection.java  Socket; send() một dòng; luồng nhận gom response nhiều dòng; log SEND/RECV
│   ├── Message.java     Tách trường; phân biệt response (bắt đầu bằng số) và event (bắt đầu bằng chữ)
│   └── Protocol.java    Hằng số, quy tắc định dạng user/pass, ghép CMD|f1|f2
├── game/
│   └── Board.java       Thế cờ đọc từ FEN (không kiểm tra luật: server là trọng tài)
└── ui/
    ├── LoginView.java   UC1–UC2  REGISTER, LOGIN
    ├── LobbyView.java   UC3–UC7  LIST_ONLINE, LEADERBOARD, FIND_MATCH, CANCEL_MATCH, LOGOUT, CREATE_ROOM
    ├── RoomView.java    UC8      INVITE, LEAVE_ROOM, INVITE_DECLINED
    ├── InviteManager.java UC9    INVITED, ACCEPT_INVITE, DECLINE_INVITE, INVITE_CANCELLED
    ├── GameView.java    UC10–UC13 MOVE, OPP_MOVE, RESIGN, OFFER/ACCEPT/DECLINE_DRAW, GAME_OVER
    ├── BoardView.java   Vẽ bàn cờ 8×8, bấm quân rồi bấm ô để đi, hỏi quân phong cấp
    ├── Texts.java       Mã lỗi / lý do kết thúc → tiếng Việt
    ├── Tables.java      Bảng cho các dòng USER / RANK
    └── Screen.java      Giao diện chung của một màn hình
```

## Các nguyên tắc

1. **Một luồng nhận** đọc socket. Giao diện không bao giờ tự đọc socket.
2. Mọi bản tin nhận được chuyển sang luồng JavaFX bằng `Platform.runLater` rồi mới cập nhật giao diện.
3. **Client không kiểm tra luật cờ.** Nước đi gửi lên server; bàn cờ luôn vẽ lại từ FEN server gửi.
4. **Mọi bản tin gửi/nhận đều in ra console** kèm thời gian (mật khẩu được che), để đối chiếu với log server khi demo.
5. Đã đăng nhập thì gửi `PING` mỗi 15 giây; server ngắt kết nối đã đăng nhập im lặng quá 45 giây.
