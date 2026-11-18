package com.chess.chessclient.net;

import java.util.ArrayList;
import java.util.List;

// Một bản tin từ server (response hoặc event), kèm các dòng dữ liệu nếu có
public class Message {
    private final String line;                          // dòng gốc
    private final String[] fields;                      // đã cắt theo '|'
    private final List<String[]> data = new ArrayList<>();   // các dòng USER/RANK (nếu có)

    public Message(String line) {
        this.line = line;
        this.fields = line.split("\\|", -1);
    }

    // Response: trường đầu là đúng 3 chữ số
    public boolean isResponse() {
        return fields[0].length() == 3 && fields[0].chars().allMatch(Character::isDigit);
    }

    public boolean isEvent() { return !isResponse(); }

    public int code() { return isResponse() ? Integer.parseInt(fields[0]) : 0; }

    // 2xx = thành công (200, 201, 202)
    public boolean isOk() { return isResponse() && code() / 100 == 2; }

    // Tên lệnh: response ở trường 1, event ở trường 0
    public String command() { return isResponse() ? fields[1] : fields[0]; }

    // Tham số thứ i (tính từ sau tên lệnh). Không có thì trả "" thay vì lỗi
    public String arg(int i) {
        int index = (isResponse() ? 2 : 1) + i;
        return index < fields.length ? fields[index] : "";
    }

    public int argCount() { return fields.length - (isResponse() ? 2 : 1); }

    // Chỉ 2 response này có dòng dữ liệu theo sau
    public boolean hasDataLines() {
        return code() == 200 && (command().equals("LIST_ONLINE") || command().equals("LEADERBOARD"));
    }

    public void addDataLine(String dataLine) { data.add(dataLine.split("\\|", -1)); }

    public List<String[]> data() { return data; }

    @Override
    public String toString() {
        String kind = isResponse() ? "RESPONSE " + code() : "EVENT";
        return kind + " " + command() + " args=" + argCount()
                + (data.isEmpty() ? "" : " +" + data.size() + " data lines");
    }
}