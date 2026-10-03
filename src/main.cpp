#include "app/application.h"
#include "app/mainwindow.h"

int main(int argc, char *argv[])
{
    AstraeaApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
