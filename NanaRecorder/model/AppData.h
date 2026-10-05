#ifndef ONLYET_APPDATA_H
#define ONLYET_APPDATA_H

#include <QReadWriteLock>
#include <QVariant>

enum class AppDataRole {
    TmpDir,     // Temporary data directory
    LogDir,     // Log directory
    RecordDir,  // Video recording directory
    RecordPath  // Absolute path to the recorded video
};

/**
* @brief Stores global properties
* @note Thread-safe
*/
class AppData
{
private:
    AppData();
    ~AppData();

#if (QT_VERSION <= QT_VERSION_CHECK(5,15,0))
	Q_DISABLE_COPY(AppData)
        AppData(AppData&&) = delete;
    AppData& operator=(AppData&&) = delete;
#else
	Q_DISABLE_COPY_MOVE(AppData)
#endif

public:
    static AppData *instance();

    void set(AppDataRole role, const QVariant &val);
    QVariant get(AppDataRole role) const;
    QString getStr(AppDataRole role) const;
    int getInt(AppDataRole role) const;
    bool getBool(AppDataRole role) const;

private:
    using Data = QMap<AppDataRole, QVariant>;
    Data m_data;
    mutable QReadWriteLock m_rwlock;
};

#define APPDATA AppData::instance()

#endif  // !ONLYET_APPDATA_H
