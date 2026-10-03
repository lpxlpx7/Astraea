#pragma once

#include <QApplication>

class AstraeaApplication final : public QApplication
{
public:
    AstraeaApplication(int &argc, char **argv);
};
