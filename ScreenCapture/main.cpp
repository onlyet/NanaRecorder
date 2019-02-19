#include <QApplication>
#include "ScreenRecordImpl.h"
//#include "ScreenRecordImpl.cpp"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	ScreenRecordImpl sr;

	return a.exec();
}
