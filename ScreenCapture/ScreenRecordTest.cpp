#include "ScreenRecordTest.h"
#include "ScreenRecordImpl.h"

ScreenRecord::ScreenRecord(QObject *parent) :
	QObject(parent)
{
	ScreenRecordImpl *sr = new ScreenRecordImpl(this);
	connect(this, SIGNAL(StartRecord()), sr, SLOT(Start()));
	Start();
}

void ScreenRecord::Start()
{
	emit StartRecord();
}

void ScreenRecord::Stop()
{
}

void ScreenRecord::Finish()
{
}
