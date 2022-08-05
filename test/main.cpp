#include <QtCore/QCoreApplication>
#include "FreeBug.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //av_freep_bug_test();
    test();

    return a.exec();
}
