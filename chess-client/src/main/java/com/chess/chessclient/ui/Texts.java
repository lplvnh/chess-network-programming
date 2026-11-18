package com.chess.chessclient.ui;

import com.chess.chessclient.net.Message;

/** Đổi mã/giá trị của giao thức sang câu tiếng Việt cho người dùng. */
public final class Texts {
    private Texts() {}

    /** Tên lỗi trong response 4xx/5xx → câu thông báo. */
    public static String error(Message m) {
        return switch (m.error()) {
            case "BAD_FORMAT"          -> "Dữ liệu gửi lên không hợp lệ";
            case "BAD_MODE"            -> "Chế độ tìm trận không hợp lệ";
            case "UNKNOWN_COMMAND"     -> "Server không hiểu lệnh này";
            case "LINE_TOO_LONG"       -> "Bản tin quá dài";
            case "NOT_LOGGED_IN"       -> "Bạn chưa đăng nhập";
            case "WRONG_STATE"         -> "Không thể thực hiện thao tác này lúc này";
            case "SERVER_ERROR"        -> "Server gặp lỗi, hãy thử lại";
            case "USERNAME_TAKEN"      -> "Tên đăng nhập đã có người dùng";
            case "INVALID_CREDENTIALS" -> "Sai tên đăng nhập hoặc mật khẩu";
            case "ALREADY_ONLINE"      -> "Tài khoản đang đăng nhập ở nơi khác";
            case "SELF_INVITE"         -> "Không thể tự mời mình";
            case "USER_NOT_ONLINE"     -> "Người này không còn online";
            case "USER_BUSY"           -> "Người này đang bận";
            case "NO_SUCH_INVITE"      -> "Lời mời không còn hiệu lực";
            case "ROOM_FULL"           -> "Phòng đã đủ người";
            case "ILLEGAL"             -> "Nước đi không hợp lệ";
            case "NOT_YOUR_TURN"       -> "Chưa đến lượt bạn";
            case "ALREADY_PENDING"     -> "Đang có lời cầu hòa chờ trả lời";
            case "WAIT_YOUR_MOVE"      -> "Hãy đi thêm một nước trước khi cầu hòa lại";
            case "NO_OFFER"            -> "Không có lời cầu hòa nào để trả lời";
            default                    -> "Lỗi " + m.code() + ": " + m.error();
        };
    }

    public static String result(String result) {
        return switch (result) {
            case "WIN"  -> "Bạn thắng!";
            case "LOSS" -> "Bạn thua.";
            case "DRAW" -> "Hòa.";
            default     -> result;
        };
    }

    public static String reason(String reason) {
        return switch (reason) {
            case "CHECKMATE"             -> "Chiếu hết";
            case "STALEMATE"             -> "Hòa pat (bên đến lượt hết nước đi)";
            case "RESIGN"                -> "Xin thua";
            case "DRAW_AGREED"           -> "Hai bên đồng ý hòa";
            case "INSUFFICIENT_MATERIAL" -> "Không đủ quân để chiếu hết";
            case "FIFTY_MOVE"            -> "Luật 50 nước";
            case "THREEFOLD_REPETITION"  -> "Lặp lại thế cờ 3 lần";
            case "DISCONNECT"            -> "Đối thủ mất kết nối";
            default                      -> reason;
        };
    }

    public static String status(String status) {
        return switch (status) {
            case "IDLE"   -> "Rảnh";
            case "QUEUED" -> "Đang tìm trận";
            case "ROOM"   -> "Trong phòng chờ";
            case "INGAME" -> "Đang chơi";
            default       -> status;
        };
    }

    public static String mode(String mode) {
        return switch (mode) {
            case "RANDOM" -> "Ngẫu nhiên";
            case "RANKED" -> "Theo trình độ";
            default       -> mode;
        };
    }

    public static String source(String source) {
        return switch (source) {
            case "RANDOM" -> "Ghép ngẫu nhiên";
            case "RANKED" -> "Ghép theo trình độ";
            case "ROOM"   -> "Phòng riêng";
            default       -> source;
        };
    }
}
