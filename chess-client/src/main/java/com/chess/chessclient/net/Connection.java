package com.chess.chessclient.net;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.time.LocalTime;
import java.time.format.DateTimeFormatter;
import java.util.function.Consumer;

/**
 * Một kết nối TCP tới server.
 *
 * <ul>
 *   <li>Gửi: {@link #send(String)} ghi đúng một dòng kết thúc bằng '\n'. Có khóa (synchronized)
 *       vì luồng giao diện và nhịp PING cùng gửi.</li>
 *   <li>Nhận: một luồng riêng ({@link #startReceiver}) đọc từng dòng, gom response nhiều dòng
 *       (200|LIST_ONLINE|n và 200|LEADERBOARD|k|... kèm n/k dòng dữ liệu) thành một bản tin trọn vẹn.</li>
 *   <li>Mọi bản tin gửi và nhận đều được in ra console (mật khẩu được che).</li>
 * </ul>
 */
public class Connection {
    private static final DateTimeFormatter TS = DateTimeFormatter.ofPattern("HH:mm:ss.SSS");

    private final Socket socket;
    private final BufferedReader in;
    private final Writer out;
    private volatile boolean closed;

    private Connection(Socket socket) throws IOException {
        this.socket = socket;
        this.in = new BufferedReader(new InputStreamReader(socket.getInputStream(), StandardCharsets.US_ASCII));
        this.out = new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.US_ASCII);
    }

    /** Mở kết nối, chờ tối đa timeoutMs. */
    public static Connection open(String host, int port, int timeoutMs) throws IOException {
        Socket s = new Socket();
        s.connect(new InetSocketAddress(host, port), timeoutMs);
        s.setTcpNoDelay(true);                       // bản tin ngắn: gửi ngay, không gom
        Connection c = new Connection(s);
        log("CONN", host + ":" + port);
        return c;
    }

    /** Gửi một dòng. Tự thêm '\n' (không dùng println vì Windows sẽ thêm "\r\n"). */
    public synchronized void send(String line) throws IOException {
        if (line.indexOf('\n') >= 0 || line.indexOf('\r') >= 0)
            throw new IllegalArgumentException("Bản tin không được chứa xuống dòng");
        if (line.length() > Protocol.MAX_LINE)
            throw new IllegalArgumentException("Bản tin dài quá " + Protocol.MAX_LINE + " byte");
        log("SEND", mask(line));
        out.write(line);
        out.write('\n');
        out.flush();
    }

    /** Đọc trọn một bản tin. Trả null khi server đóng kết nối. */
    public Message readMessage() throws IOException {
        String first = readLine();
        if (first == null) return null;
        Message m = Message.parse(first);
        int n = m.expectedDataLines();
        for (int i = 0; i < n; i++) {
            String line = readLine();
            if (line == null) return null;            // đứt giữa khối: coi như mất kết nối
            m.addDataLine(line);
        }
        return m;
    }

    private String readLine() throws IOException {
        String line = in.readLine();                  // BufferedReader tự tách theo '\n' (và bỏ '\r')
        if (line != null) log("RECV", line);
        return line;
    }

    /**
     * Chạy luồng nhận. Mỗi bản tin trọn vẹn được đưa cho onMessage (trên luồng nhận,
     * giao diện phải tự chuyển sang luồng JavaFX). Khi kết nối đứt mà không phải do
     * chính client đóng thì gọi onClosed đúng một lần.
     */
    public void startReceiver(Consumer<Message> onMessage, Runnable onClosed) {
        Thread t = new Thread(() -> {
            try {
                Message m;
                while ((m = readMessage()) != null) onMessage.accept(m);
            } catch (IOException | RuntimeException e) {
                if (!closed) log("ERR ", e.toString());
            } finally {
                boolean byUs = closed;
                close();
                if (!byUs) {
                    log("DISC", "server closed the connection");
                    onClosed.run();
                }
            }
        }, "receiver");
        t.setDaemon(true);                            // tắt cửa sổ là luồng tự dừng
        t.start();
    }

    public void close() {
        closed = true;
        try { socket.close(); } catch (IOException ignored) { }
    }

    public boolean isClosed() { return closed; }

    // ---------- log ra console ----------

    private static void log(String dir, String text) {
        System.out.println("[" + LocalTime.now().format(TS) + "] " + dir + "  " + text);
    }

    /** LOGIN|alice|secret|RANKED → LOGIN|alice|***|RANKED (giống log của server). */
    static String mask(String line) {
        if (line.startsWith("LOGIN|") || line.startsWith("REGISTER|")) {
            String[] f = line.split("\\|", -1);
            if (f.length >= 3) {
                f[2] = "***";
                return String.join("|", f);
            }
        }
        return line;
    }
}
