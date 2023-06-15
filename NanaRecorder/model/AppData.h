#ifndef ONLYET_APPDATA_H
#define ONLYET_APPDATA_H

#include <QReadWriteLock>
#include <QVariant>

enum class AppDataRole {
    TmpDir,     // 临时数据目录
    LogDir,     // 日志目录
    RecordDir,  // 录制视频目录
    RecordPath  // 录制视频绝对路径
};

/**
 * @brief 存放全局属性
 * @note 线程安全
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
