#include "NanaRecorder.h"

#include <log.h>
#include <dump.h>

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 初始化日志
    //LogInit(/*logpath*/ qApp->applicationDirPath(), a.applicationVersion());

    // 初始化dump生成器
    Dump::Init(/*QString("%1/dump").arg(tmppath)*/ qApp->applicationDirPath());

    NanaRecorder w;
    w.show();
    return a.exec();
}
