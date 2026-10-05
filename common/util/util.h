#ifndef ONLYET_UTIL_H
#define ONLYET_UTIL_H

#include <QString>
#include <QVariant>
#include <QSettings>
#include <QEventLoop>
#include <QJsonObject>

#define APPNAME "XXX"
#define qstr QStringLiteral
#define DATETIME_FORMAT_DEFAULT         "yyyy-MM-dd hh:mm:ss"

class QWidget;

namespace onlyet {

/*! @brief Toolset */
namespace util {
/*! @brief Get the current formatted time */
QString currentDateTimeString(const QString& format = DATETIME_FORMAT_DEFAULT);

/*! @brief Read the specified configuration from the configuration file */
QVariant getSetting(const QString& key, const QVariant& defaultValue = QVariant(), const QString& filename = "");
/*! @brief Modify configuration */
void setSetting(const QString& key, const QVariant& value, const QString& filename = "");

/*! @brief Extract the substring located between A and B from the src string */
QString getPartBetween(const QString& src, const QString& A, const QString& B);
/*! @brief Extracts the substring between A and B from the source string; if B is not found, returns the substring from A to the end. */
QString getPartBetweenEx(const QString& src, const QString& A, const QString& B);

/*! @brief Convert a byte count into a human-readable format */
QString parseBytesReadable(qint64 bytes);
/*! @brief Converts a duration in seconds into a human-readable format */
QString parseSecsReadable(int secs);

/**
* @brief Calculates the MD5 hash of a string.
* @param[in] in    Input string. Must be a QByteArray; the caller determines the character encoding.
* @param[in] type  Type: 0 for 32-character MD5, 1 for 16-character MD5.
* @return The MD5 hash.
*/
QString md5(const QByteArray& in, int type = 0);

/*! @brief Starts an event loop in the current thread, waits for msecs milliseconds, and returns the QEventLoop execution result. */
int esleep(QEventLoop* loop, int msecs);

/*! @brief Get the HTML string for vertically aligned image and text */
QString getHtmlIconTextVertical(const QString& icon, const QString& text);

/*! @brief Ensures only one instance of the program exists; returns false if an instance already exists. */
bool setProgramUnique(const QString& name);

/*! @brief Checks the full file path and returns a valid full file path. Creates the directory if it does not exist. 'isCover' indicates whether to overwrite existing files. */
QString checkFile(const QString& filepath, bool isCover = false);

/*! @brief Recursively delete a directory. Cannot delete files or directories currently in use. */
bool rmDir(const QString& path);

/*! @brief Converts a QJsonObject to a string */
QString Json2String(const QJsonObject& json);

/*! @brief Converts a string to a QJsonObject */
QJsonObject String2Json(const QString& data, QString* err = Q_NULLPTR);

qint64 mSecsSinceEpoch();

#if 0
    QString localIpv4();
#endif

/**
* @brief Ensures the directory exists; creates it if it does not.
* @param dirPath
* @return Returns false only if directory creation fails.
*/
bool ensureDirExist(const QString& dirPath);

int screenWidth();
int screenHeight();

int   scaleWidthByResolution(int width);
QSize newSize(QSize size);

QString     formatTime(int secs, const QString& format);
QStringList secToTime(int secs);
QString     timestamp2String(qint64 ms);

QString  QVariant2QString(const QVariant& map);
QVariant QString2QVariant(const QString& s);

/**
* @brief Checks if the drive letter exists
* @param drive Drive letter
* @return
*/
bool isDriveExist(const QString& drive);

/**
* @brief Checks if the file exists
* @param path File path
* @return
*/
bool isFileExist(const QString& path);

/**
* @brief Checks if the directory exists
* @param path Directory path
* @return
*/
bool isDirExist(const QString& path);

QStringList filePathListInDir(const QString& dirPath, QStringList filter = QStringList());

/**
* @brief Returns the directory containing the application executable.
* @return
*/
QString appDirPath();

/**
* @brief Prevents QProcess from failing to execute paths containing spaces
* @param exePath Path containing spaces
* @return Executable path
*/
QString getExecutableExePath(const QString& exePath);

void setRetainSizeWhenHidden(QWidget* w, bool isRetain = true);

// Delete the sub-level files of the directory that satisfies nameFilter.
bool removeFile(const QString& dirPath, const QString& nameFilter);

/**
* @brief Fixes a bug where QCoreApplication::applicationName() returns an empty string in certain Windows 7 environments, preventing the configuration file from being read.
* @param argv0
* @return
*/
QString getAppName(const QString& argv0);

// Check if there is sufficient disk space
bool isDiskSpaceEnough(QString dir = "");
};  // namespace util

}  // namespace onlyet

#endif // ONLYET_UTIL_H
