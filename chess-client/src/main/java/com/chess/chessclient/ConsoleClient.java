package com.chess.chessclient;

import com.chess.chessclient.net.Connection;
import com.chess.chessclient.net.Protocol;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

/**
 * Client dạng dòng lệnh (giống nc) để thử tầng mạng, không cần giao diện.
 * Chạy: ConsoleClient [host] [port], rồi gõ bản tin, ví dụ LOGIN|alice|secret12|RANKED.
 * Mọi bản tin gửi/nhận được in ra kèm thời gian; response nhiều dòng được gom thành một bản tin.
 */
public class ConsoleClient {
    public static void main(String[] args) throws IOException {
        String host = args.length > 0 ? args[0] : "localhost";
        int port = args.length > 1 ? Integer.parseInt(args[1]) : Protocol.DEFAULT_PORT;

        Connection conn = Connection.open(host, port, 5000);
        conn.startReceiver(
                m -> System.out.println("   => " + (m.isResponse() ? "RESPONSE " + m.code() : "EVENT")
                        + " " + m.name() + (m.data().isEmpty() ? "" : " +" + m.data().size() + " dòng")),
                () -> { System.out.println("Server đã đóng kết nối"); System.exit(0); });

        BufferedReader kb = new BufferedReader(new InputStreamReader(System.in));
        String line;
        while ((line = kb.readLine()) != null) {
            if (line.isBlank()) continue;
            try {
                conn.send(line.strip());
            } catch (IllegalArgumentException e) {
                System.out.println("Không gửi: " + e.getMessage());
            }
        }
        conn.close();
    }
}
