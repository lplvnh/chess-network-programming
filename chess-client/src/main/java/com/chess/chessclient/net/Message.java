package com.chess.chessclient.net;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Một bản tin server gửi xuống, đã tách trường.
 *
 * <pre>
 * response = response-line *data-line     ví dụ 200|LOGIN|alice|1200|RANKED
 * event    = event-name *( "|" field )     ví dụ OPP_MOVE|e2e4|NONE|&lt;fen&gt;
 * </pre>
 *
 * Response bắt đầu bằng mã 3 chữ số, event bắt đầu bằng chữ cái.
 * Với response, {@link #name()} là tên lệnh đang được trả lời (trường thứ 2);
 * với event, {@link #name()} là tên event (trường đầu).
 * {@link #arg(int)} đánh số các trường còn lại từ 0.
 */
public final class Message {

    public enum Kind { RESPONSE, EVENT }

    private final String raw;
    private final String[] fields;
    private final Kind kind;
    private final List<String[]> data = new ArrayList<>();

    private Message(String raw) {
        this.raw = raw;
        this.fields = raw.split("\\|", -1);          // -1: giữ cả trường rỗng
        char c = raw.isEmpty() ? ' ' : raw.charAt(0);
        this.kind = Character.isDigit(c) ? Kind.RESPONSE : Kind.EVENT;
    }

    public static Message parse(String line) {
        return new Message(line);
    }

    public Kind kind()          { return kind; }
    public boolean isResponse() { return kind == Kind.RESPONSE; }
    public boolean isEvent()    { return kind == Kind.EVENT; }

    /** Mã trả lời (200, 404...). Event trả về 0. */
    public int code() {
        if (!isResponse()) return 0;
        try { return Integer.parseInt(fields[0]); }
        catch (NumberFormatException e) { return 0; }
    }

    /** Response 2xx. */
    public boolean isOk() {
        int c = code();
        return c >= 200 && c < 300;
    }

    /** Response: tên lệnh được trả lời. Event: tên event. */
    public String name() {
        if (isResponse()) return fields.length > 1 ? fields[1] : "";
        return fields[0];
    }

    /** Trường thứ i sau tên. Không có thì trả "" (không ném lỗi). */
    public String arg(int i) {
        int idx = (isResponse() ? 2 : 1) + i;
        return idx < fields.length ? fields[idx] : "";
    }

    public int argCount() {
        return Math.max(0, fields.length - (isResponse() ? 2 : 1));
    }

    public int argInt(int i, int fallback) {
        try { return Integer.parseInt(arg(i)); }
        catch (NumberFormatException e) { return fallback; }
    }

    /** Tên lỗi của response 4xx/5xx, ví dụ "ILLEGAL". */
    public String error() {
        return isOk() ? "" : arg(0);
    }

    /** Các dòng dữ liệu (USER|... hoặc RANK|...), mỗi dòng đã tách trường; phần tử 0 là "USER"/"RANK". */
    public List<String[]> data() {
        return Collections.unmodifiableList(data);
    }

    /**
     * Số dòng dữ liệu đi kèm: chỉ 200|LIST_ONLINE|n và 200|LEADERBOARD|k|... có,
     * số đếm nằm ở trường thứ 3 (arg 0).
     */
    int expectedDataLines() {
        if (code() != 200) return 0;
        if (!name().equals("LIST_ONLINE") && !name().equals("LEADERBOARD")) return 0;
        int n = argInt(0, 0);
        return Math.max(0, Math.min(n, 10_000));   // chặn số vô lý
    }

    void addDataLine(String line) {
        data.add(line.split("\\|", -1));
    }

    public String raw() { return raw; }

    @Override
    public String toString() {
        return data.isEmpty() ? raw : raw + " (+" + data.size() + " dòng dữ liệu)";
    }
}
