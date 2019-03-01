#pragma once
#include <QObject>

class ScreenRecord : public QObject
{
	Q_OBJECT
public:
	ScreenRecord(QObject *parent = Q_NULLPTR);

};