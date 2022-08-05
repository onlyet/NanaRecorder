#include "NanaRecorder.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NanaRecorder w;
    w.show();
    return a.exec();
}
