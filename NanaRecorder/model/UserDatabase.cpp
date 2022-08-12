#include "UserDatabase.h"

//#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <AppData.h>
#include <util.h>

#include <QTime>

UserDatabase* UserDatabase::instance()
{
    static UserDatabase s_inst;
    return &s_inst;
}

UserDatabase::UserDatabase(QObject *parent)
    : IDatabase(parent)
{
    qDebug() << "UserDatabase()";
}

UserDatabase::~UserDatabase()
{
    qDebug() << "~UserDatabase()";
}
