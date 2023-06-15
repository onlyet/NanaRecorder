#include "NanaRecorder.h"

#include <log.h>
#include <dump.h>
#include <util.h>
#include <AppData.h>

#include <QtWidgets/QApplication>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString appName = onlyet::util::getAppName(argv[0]);

    //使程序唯一
    if (!onlyet::util::setProgramUnique(appName)) {
        return 0;
    }

    //a.setStyleSheet("file:///:/qss/XXX.qss");

	auto ad = AppData::instance();

    // 创建临时文件夹
    const auto docpath   = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const auto tmppath   = QString("%1/%2").arg(docpath, appName);
    const auto logpath   = QString("%1/log").arg(tmppath);
    const auto videopath = QString("%1/video").arg(tmppath);
    const auto dumppath  = QString("%1/dump").arg(tmppath);
    ad->set(AppDataRole::TmpDir, tmppath);
    ad->set(AppDataRole::LogDir, logpath);
    ad->set(AppDataRole::RecordDir, videopath);
    QDir dir(docpath);
    dir.mkpath(logpath);
    dir.mkpath(videopath);

    // 初始化日志
    LogInit(logpath, a.applicationVersion());
    setLogLevel(QtDebugMsg);

    // 初始化dump生成器
    Dump::Init(dumppath);

#if 0
	// 初始化配置项
	APPCFG->init(QString("%1/%2.ini").arg(util::appDirPath(), util::getAppName(argv[0])));
	USERCFG->init(QString("%1/user.ini").arg(tmppath));

    a.setWindowIcon(QIcon(":/NanaRecorder/image/momo1.ico"));
#endif

    NanaRecorder w;
    w.show();
    return a.exec();
}
