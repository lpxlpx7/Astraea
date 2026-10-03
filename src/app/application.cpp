#include "application.h"

#include "theme.h"
#include <QFont>

AstraeaApplication::AstraeaApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setApplicationName(QStringLiteral("Astraea"));
    setApplicationDisplayName(QStringLiteral("Astraea"));
    setOrganizationName(QStringLiteral("Astraea"));
    QFont font(QStringLiteral("Segoe UI"), 10);
    font.setStyleStrategy(QFont::PreferAntialias);
    setFont(font);
    setStyleSheet(AstraeaTheme::styleSheet());
}
