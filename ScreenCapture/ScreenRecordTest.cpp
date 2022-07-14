#include "ScreenRecordTest.h"
#include "ScreenRecordImpl.h"
#include <QTimer>
#include <QApplication>
#include <QDesktopWidget>

ScreenRecord::ScreenRecord(QObject *parent) :
    QObject(parent)
{
    ScreenRecordImpl *sr = new ScreenRecordImpl(this);
    QVariantMap args;
    args["filePath"] = "test.mp4";
    //args["width"] = 1920;
    //args["height"] = 1080;
    args["width"] = QApplication::desktop()->screenGeometry().width();
    args["height"] = QApplication::desktop()->screenGeometry().height();
    args["fps"] = 25;
    args["audioBitrate"] = 128000;

    sr->Init(args);

    QTimer::singleShot(1000, sr, SLOT(Start()));
    QTimer::singleShot(11000, sr, SLOT(Stop()));

    //QTimer::singleShot(5000, sr, SLOT(Pause()));
    //QTimer::singleShot(10000, sr, SLOT(Pause()));
    //QTimer::singleShot(15000, sr, SLOT(Start()));

    //QTimer::singleShot(30000, sr, SLOT(Stop()));
}
