#include "rfbss.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RFBSS w;
    w.show();

    return a.exec();
}
