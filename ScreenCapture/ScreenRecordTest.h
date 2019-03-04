#pragma once
#include <QObject>
#include <QVariant>

class ScreenRecord : public QObject
{
	Q_OBJECT
public:
	ScreenRecord(QObject *parent = Q_NULLPTR);

private:
	QVariantMap m_args;
};