package com.chess.chessclient.net;

import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.function.Consumer;

// Một kết nối TCP tới server: gửi 1 bản tin, đọc 1 bản tin, in log mọi bản tin.
//
// Log để chứng minh implement đúng giao thức:
//  - SEND in NGAY TRƯỚC khi ghi ra socket, RECV in NGAY KHI nhận đủ một bản tin (trước khi xử lý)
//  - in NGUYÊN VĂN đúng các byte gửi/nhận, kể cả '\n': không che mật khẩu, không cắt, không ghép
//  - response nhiều dòng (LIST_ONLINE, LEADERBOARD) là MỘT bản tin nên in thành MỘT mục
//  - cùng định dạng với server.log để đặt cạnh nhau đối chiếu
public class Connection {
    private static final DateTimeFormatter TS = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss.SSS");

    private Socket socket;
    private InputStream in;       // đọc từng byte (để giữ nguyên '\r', '\n' cho log)
    private OutputStream out;     // ghi đúng mảng byte đã in ra log
    private volatile boolean closedByUs;   // client tự đóng (không phải mất kết nối)
    private volatile boolean ended;        // luồng nhận đã thấy kết nối kết thúc (đã log DISC)

    public void connect(String host, int port) throws IOException {
        closedByUs = false;            // ChessApp dùng lại đối tượng này khi kết nối lại
        ended = false;
        socket = new Socket();
        socket.connect(new InetSocketAddress(host, port), 5000);   // chờ tối đa 5 giây
        in  = new BufferedInputStream(socket.getInputStream());
        out = new BufferedOutputStream(socket.getOutputStream());
        logEvent("CONN", "connected to " + host + ":" + port + " (local port " + socket.getLocalPort() + ")");
    }

    // synchronized: nhiều luồng cùng gọi send() cũng không bị trộn chữ
    public synchronized void send(String line) throws IOException {
        byte[] data = (line + "\n").getBytes(StandardCharsets.UTF_8);   // tự thêm '\n', KHÔNG dùng println
        logMsg("SEND", data, data.length);   // in đúng mảng byte sắp gửi...
        out.write(data);                     // ...rồi mới ghi ra socket
        out.flush();                         // đẩy đi ngay, không để nằm trong bộ đệm
    }

    // Đọc từng byte tới '\n'. Byte thô (kể cả '\r', '\n') được nối vào raw để in nguyên văn;
    // giá trị trả về là nội dung dòng đã bỏ '\n' (và '\r' trước nó) để tách trường.
    // Trả null nếu kết nối đóng trước khi gặp '\n'.
    private String readLine(ByteArrayOutputStream raw) throws IOException {
        int start = raw.size();
        int b;
        while ((b = in.read()) != -1) {
            raw.write(b);
            if (b == '\n') break;
        }
        if (b == -1) return null;
        byte[] all = raw.toByteArray();
        int end = all.length - 1;                                   // bỏ '\n'
        if (end > start && all[end - 1] == '\r') end--;             // bỏ '\r' nếu có
        return new String(all, start, end - start, StandardCharsets.UTF_8);
    }

    // Đọc MỘT bản tin hoàn chỉnh (gom thêm n dòng nếu là LIST_ONLINE/LEADERBOARD),
    // in MỘT mục RECV ngay khi đủ, trước khi trả về cho giao diện xử lý
    public Message readMessage() throws IOException {
        ByteArrayOutputStream raw = new ByteArrayOutputStream();     // byte thô của CẢ bản tin
        String line = readLine(raw);
        if (line == null) { logPartial(raw); return null; }         // server đóng kết nối
        Message msg = new Message(line);
        if (msg.hasDataLines()) {
            int n = Integer.parseInt(msg.arg(0));                    // trường thứ 3 = số dòng dữ liệu
            for (int i = 0; i < n; i++) {
                String dataLine = readLine(raw);
                if (dataLine == null) { logPartial(raw); return null; }
                msg.addDataLine(dataLine);
            }
        }
        logMsg("RECV", raw.toByteArray(), raw.size());
        return msg;
    }

    // Kết nối đóng giữa chừng: vẫn in những byte đã nhận (chưa thành bản tin trọn vẹn)
    private static void logPartial(ByteArrayOutputStream raw) {
        if (raw.size() > 0) logMsg("RECV(incomplete)", raw.toByteArray(), raw.size());
    }

    // Tạo luồng nhận: mỗi bản tin gọi onMessage, mất kết nối gọi onDisconnect
    public void startReceiving(Consumer<Message> onMessage, Runnable onDisconnect) {
        Thread receiver = new Thread(() -> {
            try {
                Message msg;
                while ((msg = readMessage()) != null) onMessage.accept(msg);
                ended = true;
                logEvent("DISC", "server closed the connection");
            } catch (IOException | RuntimeException e) {
                ended = true;
                if (!closedByUs) logEvent("DISC", e.toString());
            }
            onDisconnect.run();
        }, "receiver");
        receiver.setDaemon(true);
        receiver.start();
    }

    public void close() {
        // chỉ log khi chính client chủ động đóng (đóng cửa sổ...), không log lại sau khi đã mất kết nối
        if (!closedByUs && !ended && socket != null) logEvent("DISC", "client closed the connection");
        closedByUs = true;
        try { if (socket != null) socket.close(); } catch (IOException ignored) {}
    }

    // ---------- log ra console (cùng định dạng với server.log) ----------

    // Ví dụ: 2026-11-21 14:40:01.123 SEND 28B : LOGIN|alice|secret12|RANKED\n
    private static synchronized void logMsg(String dir, byte[] data, int len) {
        System.out.println(LocalDateTime.now().format(TS) + " " + dir + " " + len + "B : " + visible(data, len));
    }

    private static synchronized void logEvent(String tag, String note) {
        System.out.println(LocalDateTime.now().format(TS) + " " + tag + " : " + note);
    }

    // Hiển thị từng byte: ký tự in được giữ nguyên; '\n' '\r' '\t' '\' và byte khác viết dạng
    // \n \r \t \\ \xHH để NHÌN THẤY được. Chỉ là cách hiển thị: nội dung và thứ tự byte không đổi.
    static String visible(byte[] data, int len) {
        StringBuilder sb = new StringBuilder(len + 8);
        for (int i = 0; i < len; i++) {
            int ch = data[i] & 0xFF;
            switch (ch) {
                case '\n' -> sb.append("\\n");
                case '\r' -> sb.append("\\r");
                case '\t' -> sb.append("\\t");
                case '\\' -> sb.append("\\\\");
                default -> {
                    if (ch < 0x20 || ch >= 0x7F) sb.append(String.format("\\x%02X", ch));
                    else sb.append((char) ch);
                }
            }
        }
        return sb.toString();
    }
}
