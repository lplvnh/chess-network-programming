package com.chess.chessclient.net;

import java.util.regex.Pattern;

/** Hằng số và quy tắc định dạng trường, theo đặc tả giao thức. */
public final class Protocol {
    private Protocol() {}

    public static final int DEFAULT_PORT = 5500;
    public static final int MAX_LINE = 512;               // byte, chưa tính '\n'
    public static final int HEARTBEAT_SECONDS = 15;       // client gửi PING mỗi 15 giây

    /** user = 3–20 ký tự A–Z a–z 0–9 _ */
    public static final Pattern USER = Pattern.compile("[A-Za-z0-9_]{3,20}");
    /** pass = 4–32 ký tự in được, không dấu cách, không '|' */
    public static final Pattern PASS = Pattern.compile("[\\x21-\\x7B\\x7D-\\x7E]{4,32}");

    public static final String MODE_RANDOM = "RANDOM";
    public static final String MODE_RANKED = "RANKED";

    /**
     * Ghép tên lệnh và các trường thành một dòng: CMD|f1|f2...
     * Từ chối trường chứa '|' hoặc xuống dòng, vì như vậy sẽ làm hỏng cú pháp bản tin.
     */
    public static String line(String command, String... fields) {
        StringBuilder sb = new StringBuilder(command);
        for (String f : fields) {
            if (f.isEmpty() || f.indexOf('|') >= 0 || f.indexOf('\n') >= 0 || f.indexOf('\r') >= 0)
                throw new IllegalArgumentException("Trường không hợp lệ: \"" + f + "\"");
            sb.append('|').append(f);
        }
        return sb.toString();
    }
}
