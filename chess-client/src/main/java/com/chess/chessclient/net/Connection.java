package com.chess.chessclient.net;

import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.function.Consumer;

// Một kết nối TCP tới server: gửi 1 dòng, đọc 1 dòng, in log mọi bản tin
public class Connection {
    private Socket socket;
    private BufferedReader in;    // đọc từng dòng
    private Writer out;           // ghi chữ

    public void connect(String host, int port) throws IOException {
        socket = new Socket();
        socket.connect(new InetSocketAddress(host, port), 5000);   // chờ tối đa 5 giây
        in  = new BufferedReader(new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8));
        out = new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8);
        System.out.println("[CONN] connected to " + host + ":" + port);
    }

    // synchronized: nhiều luồng cùng gọi send() cũng không bị trộn chữ
    public synchronized void send(String line) throws IOException {
        System.out.println("[SEND] " + line);
        out.write(line + "\n");      // tự thêm '\n', KHÔNG dùng println
        out.flush();                 // đẩy đi ngay, không để nằm trong bộ đệm
    }

    // Đứng chờ tới khi có 1 dòng. Trả null nếu server đóng kết nối
    public String readLine() throws IOException {
        String line = in.readLine();
        if (line != null) System.out.println("[RECV] " + line);
        return line;
    }

    // Đọc MỘT bản tin hoàn chỉnh (gom thêm n dòng nếu là LIST_ONLINE/LEADERBOARD)
    public Message readMessage() throws IOException {
        String line = readLine();
        if (line == null) return null;                     // server đóng kết nối
        Message msg = new Message(line);
        if (msg.hasDataLines()) {
            int n = Integer.parseInt(msg.arg(0));          // trường thứ 3 = số dòng dữ liệu
            for (int i = 0; i < n; i++) {
                String dataLine = readLine();
                if (dataLine == null) return null;
                msg.addDataLine(dataLine);
            }
        }
        return msg;
    }

    // Tạo luồng nhận: mỗi bản tin gọi onMessage, mất kết nối gọi onDisconnect
    public void startReceiving(Consumer<Message> onMessage, Runnable onDisconnect) {
        Thread receiver = new Thread(() -> {
            try {
                Message msg;
                while ((msg = readMessage()) != null) onMessage.accept(msg);
            } catch (IOException | RuntimeException e) {
                System.out.println("[DISC] " + e.getMessage());
            }
            onDisconnect.run();
        }, "receiver");
        receiver.setDaemon(true);
        receiver.start();
    }

    public void close() {
        try { if (socket != null) socket.close(); } catch (IOException ignored) {}
    }
}