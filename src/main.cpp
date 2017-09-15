#include "rfbss.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<conn_status>();
    RFBSS w;
    w.show();

    return a.exec();
}
