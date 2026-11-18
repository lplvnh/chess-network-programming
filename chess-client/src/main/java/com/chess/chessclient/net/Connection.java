package com.chess.chessclient.net;

import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

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

    public void close() {
        try { if (socket != null) socket.close(); } catch (IOException ignored) {}
    }
}