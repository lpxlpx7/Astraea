#include "theme.h"

#include <QFontDatabase>

namespace AstraeaTheme {

QString styleSheet()
{
    // Segoe UI gives the desktop shell a cleaner Windows-native text rendering style.
    return QStringLiteral(R"(
        QWidget { font-family: "Segoe UI", "Microsoft YaHei", sans-serif; color: #e8edf5; }
        QMainWindow, QWidget#root { background: #11151c; }
        QFrame#sidebar { background: #171c25; border-right: 1px solid #28313e; }
        QLabel#brand { color: #f3f6fa; font-size: 22px; font-weight: 700; }
        QLabel#brandIcon { background: transparent; }
        QLabel#eyebrow { color: #738198; font-size: 11px; font-weight: 700; letter-spacing: 1px; }
        QLabel#title { color: #f4f7fb; font-size: 28px; font-weight: 700; }
        QLabel#subtitle, QLabel#muted { color: #8592a6; }
        QPushButton { border: 0; border-radius: 7px; padding: 10px 14px; color: #aeb9c9; background: transparent; text-align: left; }
        QPushButton:hover { background: #222a37; color: #f4f7fb; }
        QPushButton:checked { background: #283b58; color: #8bc1ff; }
        QPushButton#sidebarNav { min-height: 36px; text-align: left; padding: 7px 10px; }
        QPushButton#sidebarNav[collapsed="true"] { text-align: center; padding: 7px 0px; }
        QPushButton#sidebarToggle { min-height: 36px; text-align: center; padding: 7px 6px; }
        QPushButton#primary { background: #5b8def; color: white; font-weight: 700; }
        QPushButton#primary:hover { background: #6d9cf5; }
        QPushButton#language { border: 1px solid #303b4c; border-radius: 15px; padding: 6px 12px; }
        QFrame#card { background: #191f29; border: 1px solid #27313f; border-radius: 12px; }
        QFrame#hero { background: #1b2b43; border: 1px solid #294a72; border-radius: 14px; }
        QLabel#metric { color: #f4f7fb; font-size: 24px; font-weight: 700; }
        QLabel#badge { background: #223d35; color: #76ddb2; border-radius: 10px; padding: 4px 8px; }
        QListWidget { background: transparent; border: 0; outline: 0; }
        QListWidget::item { padding: 10px; border-radius: 7px; }
        QListWidget::item:selected { background: #283b58; color: #8bc1ff; }
        QLineEdit { background: #151b24; border: 1px solid #303b4c; border-radius: 7px; padding: 9px; }
        QScrollBar:vertical { width: 8px; background: transparent; }
        QScrollBar:vertical { width: 5px; margin: 2px 0; }
        QScrollBar::handle:vertical { min-height: 24px; background: #354257; border-radius: 2px; }
    )");
}

} // namespace AstraeaTheme
