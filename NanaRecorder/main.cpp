#include "NanaRecorder.h"

#include <log.h>
#include <dump.h>

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // ��ʼ����־
    //LogInit(/*logpath*/ qApp->applicationDirPath(), a.applicationVersion());

    // ��ʼ��dump������
    Dump::Init(/*QString("%1/dump").arg(tmppath)*/ qApp->applicationDirPath());

    NanaRecorder w;
    w.show();
    return a.exec();
}
