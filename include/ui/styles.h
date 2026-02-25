#ifndef STYLES_H
#define STYLES_H

#include <QString>

namespace Ui {

inline QString mainStylesheet() {
    return
        "QWidget {"
        "    background-color: #1e1e1e;"
        "    color: #d4d4d4;"
        "    font-family: 'Segoe UI', 'Roboto', 'Arial', sans-serif;"
        "    font-size: 10pt;"
        "}"
        "QLabel {"
        "    color: #ffffff;"
        "    padding: 8px;"
        "    background-color: #252526;"
        "    border-radius: 4px;"
        "}"
        "QTreeWidget {"
        "    background-color: #252526;"
        "    border: 1px solid #3e3e42;"
        "    border-radius: 6px;"
        "    padding: 4px;"
        "    selection-background-color: #0078d4;"
        "    selection-color: #ffffff;"
        "    alternate-background-color: #2d2d30;"
        "}"
        "QTreeWidget::item {"
        "    padding: 6px;"
        "    border-radius: 3px;"
        "    min-height: 24px;"
        "}"
        "QTreeWidget::item:hover {"
        "    background-color: #2a2d2e;"
        "}"
        "QTreeWidget::item:selected {"
        "    background-color: #0078d4;"
        "    color: #ffffff;"
        "}"
        "QTreeWidget::branch {"
        "    background-color: #252526;"
        "}"
        "QHeaderView::section {"
        "    background-color: #2d2d30;"
        "    color: #ffffff;"
        "    padding: 8px;"
        "    border: none;"
        "    border-bottom: 2px solid #0078d4;"
        "    font-weight: bold;"
        "    font-size: 10pt;"
        "}"
        "QTableWidget {"
        "    background-color: #252526;"
        "    border: 1px solid #3e3e42;"
        "    border-radius: 6px;"
        "    gridline-color: #3e3e42;"
        "    selection-background-color: #0078d4;"
        "    selection-color: #ffffff;"
        "    alternate-background-color: #2d2d30;"
        "}"
        "QTableWidget::item {"
        "    padding: 6px;"
        "    border: none;"
        "}"
        "QTableWidget::item:hover {"
        "    background-color: #2a2d2e;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #0078d4;"
        "    color: #ffffff;"
        "}"
        "QTextEdit {"
        "    background-color: #252526;"
        "    border: 1px solid #3e3e42;"
        "    border-radius: 6px;"
        "    padding: 8px;"
        "    color: #d4d4d4;"
        "    selection-background-color: #0078d4;"
        "    selection-color: #ffffff;"
        "}"
        "QSplitter::handle {"
        "    background-color: #3e3e42;"
        "    width: 4px;"
        "    height: 4px;"
        "}"
        "QSplitter::handle:hover {"
        "    background-color: #0078d4;"
        "}";
}

inline QString panelHeaderStyle() {
    return
        "QLabel, QPushButton {"
        "    font-weight: bold;"
        "    font-size: 13pt;"
        "    color: #ffffff;"
        "    padding: 12px;"
        "    background-color: #0078d4;"
        "    border-radius: 6px;"
        "    border: none;"
        "}";
}

inline QString contactsButtonStyle() {
    return
        "QPushButton {"
        "    font-weight: bold;"
        "    font-size: 13pt;"
        "    color: #ffffff;"
        "    padding: 12px;"
        "    background-color: #0078d4;"
        "    border-radius: 6px;"
        "    border: none;"
        "    text-align: left;"
        "}"
        "QPushButton:hover {"
        "    background-color: #106ebe;"
        "}";
}

inline QString detailsTextEditStyle() {
    return
        "QTextEdit {"
        "    font-family: 'Consolas', 'Monaco', 'Courier New', monospace;"
        "    font-size: 9pt;"
        "    line-height: 1.4;"
        "}";
}

}

#endif
