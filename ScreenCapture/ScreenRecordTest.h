#pragma once
#include <QObject>

class ScreenRecord : public QObject
{
	Q_OBJECT
public:
	ScreenRecord(QObject *parent = Q_NULLPTR);

	void Start();
	void Stop();
	void Finish();

signals:
	void StartRecord();
	void StopRecord();
	void FinishRecord();
};