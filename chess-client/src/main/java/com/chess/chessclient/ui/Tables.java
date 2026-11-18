package com.chess.chessclient.ui;

import javafx.beans.property.SimpleStringProperty;
import javafx.scene.control.Label;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;

import java.util.function.Function;

/** Bảng hiển thị các dòng dữ liệu USER|... / RANK|... (mỗi dòng là một String[] đã tách trường). */
final class Tables {
    private Tables() {}

    static TableView<String[]> create(String emptyText) {
        TableView<String[]> t = new TableView<>();
        t.setColumnResizePolicy(TableView.CONSTRAINED_RESIZE_POLICY_FLEX_LAST_COLUMN);
        t.setPlaceholder(new Label(emptyText));
        return t;
    }

    /** Thêm cột hiển thị trường thứ index của dòng (0 là "USER"/"RANK"). */
    static void column(TableView<String[]> t, String title, int index) {
        column(t, title, index, Function.identity());
    }

    static void column(TableView<String[]> t, String title, int index, Function<String, String> show) {
        TableColumn<String[], String> col = new TableColumn<>(title);
        col.setCellValueFactory(d -> {
            String[] row = d.getValue();
            return new SimpleStringProperty(index < row.length ? show.apply(row[index]) : "");
        });
        col.setSortable(false);
        t.getColumns().add(col);
    }
}
