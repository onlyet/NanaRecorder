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

/*! @brief 工具集 */
namespace util {
/*! @brief 获取当前格式化时间 */
QString currentDateTimeString(const QString& format = DATETIME_FORMAT_DEFAULT);

/*! @brief 从配置文件中读取指定配置 */
QVariant getSetting(const QString& key, const QVariant& defaultValue = QVariant(), const QString& filename = "");
/*! @brief 修改配置 */
void setSetting(const QString& key, const QVariant& value, const QString& filename = "");

/*! @brief 从src字符串中截取介于A和B之间的字符子串 */
QString getPartBetween(const QString& src, const QString& A, const QString& B);
/*! @brief 从src字符串中截取介于A和B之间的字符子串，如果不存在B，则返回A到结尾的子串 */
QString getPartBetweenEx(const QString& src, const QString& A, const QString& B);

/*! @brief 将字节数转化为易读的表示 */
QString parseBytesReadable(qint64 bytes);
/*! @brief 将秒数转化为易读的表示 */
QString parseSecsReadable(int secs);

/**
     * @brief 计算字符串md5
     * @param[in] in    输入字符串. 要求为QByteArray, 调用者自己决定字符编码
     * @param[in] type  类型, 0-32位md5, 1-16位md5
     * @return 返回md5
     * */
QString md5(const QByteArray& in, int type = 0);

/*! @brief 当前线程开启事件循环等待msecs毫秒, 返回QEventLoop执行结果 */
int esleep(QEventLoop* loop, int msecs);

/*! @brief 获取垂直对齐的图片文字的HTML字符串 */
QString getHtmlIconTextVertical(const QString& icon, const QString& text);

/*! @brief 使程序唯一存在，如果已存在返回false */
bool setProgramUnique(const QString& name);

/*! @brief 检查完整文件路径，返回合理的完整文件路径。如果所属路径不存在则创建。isCover表示是否删除已有 */
QString checkFile(const QString& filepath, bool isCover = false);

/*! @brief 循环删除目录. 无法删除正在使用的文件和目录 */
bool rmDir(const QString& path);

/*! @brief 将QJsonObject转为字符串 */
QString Json2String(const QJsonObject& json);

/*! @brief 将字符串转为QJsonObject */
QJsonObject String2Json(const QString& data, QString* err = Q_NULLPTR);

qint64 mSecsSinceEpoch();

#if 0
    QString localIpv4();
#endif

/**
     * @brief 确保目录存在，不存在则创建
     * @param dirPath
     * @return 创建目录失败才返回false
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
     * @brief 判断盘符是否存在
     * @param drive 盘符
     * @return
     */
bool isDriveExist(const QString& drive);

/**
     * @brief 判断文件是否存在
     * @param path 文件路径
     * @return
     */
bool isFileExist(const QString& path);

/**
     * @brief 判断目录是否存在
     * @param path 目录路径
     * @return
     */
bool isDirExist(const QString& path);

QStringList filePathListInDir(const QString& dirPath, QStringList filter = QStringList());

/**
     * @brief 返回app exe所在目录
     * @return
     */
QString appDirPath();

/**
     * @brief 避免QProcess不能执行带空格的路径
     * @param exePath 带空格的路径
     * @return 可执行的路径
     */
QString getExecutableExePath(const QString& exePath);

void setRetainSizeWhenHidden(QWidget* w, bool isRetain = true);

//删除满足nameFilter的目录的下一级文件
bool removeFile(const QString& dirPath, const QString& nameFilter);

/**
     * @brief 修复某win7环境下QCoreApplication::applicationName()返回空的造成读取不了配置文件的bug
     * @param argv0 
     * @return 
    */
QString getAppName(const QString& argv0);

// 检查磁盘空间是否足够
bool isDiskSpaceEnough(QString dir = "");
};  // namespace util

}  // namespace onlyet

#endif // ONLYET_UTIL_H
