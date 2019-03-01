#include "ScreenRecordTest.h"
#include "ScreenRecordImpl.h"
#include <QTimer>

ScreenRecord::ScreenRecord(QObject *parent) :
	QObject(parent)
{
	ScreenRecordImpl *sr = new ScreenRecordImpl(this);
	QTimer::singleShot(1000, sr, SLOT(Start()));
	//QTimer::singleShot(10000, sr, SLOT(Pause()));
	QTimer::singleShot(11000, sr, SLOT(Stop()));
}
