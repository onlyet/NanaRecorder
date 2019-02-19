#include <QApplication>
#include "ScreenRecordImpl.h"
#include "ScreenRecordTest.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	ScreenRecord sr;

	return a.exec();
}
