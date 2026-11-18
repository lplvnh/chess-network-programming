package com.chess.chessclient;

import com.chess.chessclient.net.Connection;
import java.io.IOException;
import java.util.Scanner;
import com.chess.chessclient.net.Message;

// Client dạng console, giống nc, để test phần mạng
public class ConsoleTest {
    public static void main(String[] args) throws IOException {
        String host = args.length > 0 ? args[0] : "localhost";
        Connection conn = new Connection();
        conn.connect(host, 5500);

        conn.startReceiving(
                msg -> {                                             // mỗi bản tin
                    System.out.println("   => " + msg);
                    for (String[] row : msg.data()) System.out.println("      " + String.join(" ", row));
                },
                () -> {                                              // mất kết nối
                    System.out.println("[DISC] disconnected");
                    System.exit(0);
                });

        Scanner keyboard = new Scanner(System.in);
        while (keyboard.hasNextLine()) {
            String line = keyboard.nextLine();
            if (!line.isEmpty()) conn.send(line);
        }
        conn.close();
    }
}