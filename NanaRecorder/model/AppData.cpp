#include "AppData.h"

AppData::AppData() = default;
AppData::~AppData() = default;

AppData *AppData::instance()
{
    static AppData inst;
    return &inst;
}

void AppData::set(AppDataRole role, const QVariant &val)
{
    QWriteLocker locker(&m_rwlock);
    m_data.insert(role, val);
}

QVariant AppData::get(AppDataRole role) const
{
    QReadLocker locker(&m_rwlock);
    return m_data.value(role);
}

QString AppData::getStr(AppDataRole role) const
{
    return get(role).toString();
}

int AppData::getInt(AppDataRole role) const
{
    return get(role).toInt();
}

bool AppData::getBool(AppDataRole role) const
{
    return get(role).toBool();
}
