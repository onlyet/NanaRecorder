#ifndef USERDATABASE_H
#define USERDATABASE_H

#include <IDatabase.h>
#include <QObject>
#include <QVariant>

class UserDatabase : public IDatabase
{
    Q_OBJECT
public:
    static UserDatabase* instance();

    explicit UserDatabase(QObject *parent = nullptr);
    ~UserDatabase();

public slots:

signals:

private:
};

#define USERDB UserDatabase::instance()

#endif // USERDATABASE_H

