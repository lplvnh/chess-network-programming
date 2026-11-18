package com.chess.chessclient;

import com.chess.chessclient.net.Connection;
import java.io.IOException;
import java.util.Scanner;

// Client dạng console, giống nc, để test phần mạng
public class ConsoleTest {
    public static void main(String[] args) throws IOException {
        String host = args.length > 0 ? args[0] : "localhost";
        Connection conn = new Connection();
        conn.connect(host, 5500);

        // Luồng NHẬN: đọc mãi cho tới khi server đóng kết nối
        Thread receiver = new Thread(() -> {
            try {
                while (conn.readLine() != null) { }        // readLine tự in [RECV]
                System.out.println("[DISC] server closed the connection");
            } catch (IOException e) {
                System.out.println("[DISC] " + e.getMessage());
            }
            System.exit(0);
        });
        receiver.setDaemon(true);    // luồng phụ: chương trình tắt thì nó tắt theo
        receiver.start();

        // Luồng CHÍNH: đọc bàn phím, gửi lên server
        Scanner keyboard = new Scanner(System.in);
        while (keyboard.hasNextLine()) {
            String line = keyboard.nextLine();
            if (line.isEmpty()) continue;
            conn.send(line);
        }
        conn.close();
    }
}