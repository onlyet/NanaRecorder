#include "ScreenRecordTest.h"
#include "ScreenRecordImpl.h"
#include <QTimer>

ScreenRecord::ScreenRecord(QObject *parent) :
	QObject(parent)
{
	ScreenRecordImpl *sr = new ScreenRecordImpl(this);
	QVariantMap args;
	args["filePath"] = "test.mp4";
	//args["width"] = 1920;
	//args["height"] = 1080;
	args["width"] = 1440;
	args["height"] = 900;
	args["fps"] = 30;
	args["audioBitrate"] = 128000;

	sr->Init(args);

	QTimer::singleShot(1000, sr, SLOT(Start()));
	//QTimer::singleShot(5000, sr, SLOT(Pause()));
	QTimer::singleShot(11000, sr, SLOT(Stop()));
}
