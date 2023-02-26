#include "util.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QMutex>
#include <QSettings>
#include <QCryptographicHash>
#include <math.h>
#include <QTimer>
#include <QLocalSocket>
#include <QLocalServer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QApplication>
#include  <QNetworkInterface>
#include <QProcess>
#include <QScreen>
#include <QLabel>
#include <QStringList>
#include <QStorageInfo>
#include <QStandardPaths>

#ifdef WIN32
#include <DXGI.h>
#endif // WIN32

QString util::currentDateTimeString(const QString &format)
{
    return QDateTime::currentDateTime().toString(format);
}

QVariant util::getSetting(const QString &key, const QVariant &defaultValue, const QString &filename)
{
    QString path;
    QString appName = APPNAME;
    QString exeDirPath = QApplication::applicationDirPath();
    if (filename.isEmpty())
    {
        path = QString("%1/%2.ini").arg(exeDirPath).arg(appName);
    }
    else
    {
        path = filename;
    }

    QSettings mSetting(path, QSettings::IniFormat);
#if (QT_VERSION <= QT_VERSION_CHECK(6,0,0))
    mSetting.setIniCodec("UTF-8");
#endif
    return mSetting.value(key, defaultValue);
}

void util::setSetting(const QString &key, const QVariant &value, const QString &filename)
{
    QString path = filename;
    if (path.isEmpty())
    {
        path = APPNAME;
    }
	path = QString("%1/%2.ini").arg(QApplication::applicationDirPath()).arg(path);
    QSettings mSetting(path, QSettings::IniFormat);
#if (QT_VERSION <= QT_VERSION_CHECK(6,0,0))
    mSetting.setIniCodec("UTF-8");
#endif
    mSetting.setValue(key, value);
}

QString util::getPartBetween(const QString &src, const QString &A, const QString &B)
{
    int idxA = src.indexOf(A);
    if (idxA >= 0)
    {
        idxA += A.length();
        int idxB = src.indexOf(B, idxA);
        if (idxB > idxA)
        {
            return src.mid(idxA, idxB-idxA);
        }
    }
    return "";
}

QString util::getPartBetweenEx(const QString &src, const QString &A, const QString &B)
{
    int idxA = src.indexOf(A);
    if (idxA >= 0)
    {
        idxA += A.length();
        int idxB = src.indexOf(B, idxA);
        if (idxB > idxA)
        {
            return src.mid(idxA, idxB-idxA);
        }
        else
        {
            return src.mid(idxA);
        }
    }
    return "";
}


QString util::parseBytesReadable(qint64 bytes)
{
    QString strSize = "0 B";
    if (bytes == 0)
    {
        return strSize;
    }

    bool isNegative = false;
    if (bytes < 0)
    {
        isNegative = true;
        bytes = -bytes;
    }

    const QStringList units = {"B", "KB", "MB", "GB", "TB"};

    int i = 0;
    for(; i < units.size(); ++i)
    {
        if(bytes < pow(1024, i))
        {
            break;
        }
    }
    --i;
    qreal ret = bytes/pow(1024, i);
    strSize = QString("%1 %2").arg(QString::number(ret,'f',2)).arg(units.value(i));

    if (isNegative) strSize = QString("-%1").arg(strSize);

//    qDebug() << "bytes:" << bytes << "size:" << strSize;
    return strSize;
}

QString util::parseSecsReadable(int secs)
{
    QString ret = "";
    if (secs < 0)
    {
        secs = -secs;
        ret = "-";
    }

    int days = secs/86400;
    if (days > 0)
    {
        ret = QString("%1%2:").arg(ret).arg(secs);
    }

    secs = secs%86400;
    QTime tt(0,0,0);
    tt = tt.addSecs(secs);
    ret = QString("%1%2").arg(ret).arg(tt.toString("hh:mm:ss"));
    return ret;
}

QString util::md5(const QByteArray &in, int type)
{
    QByteArray ret = QCryptographicHash::hash(in, QCryptographicHash::Md5).toHex();
    if(type == 1)
    {
        ret = ret.mid(8,16);
    }
    return ret;
}

int util::esleep(QEventLoop *loop, int msecs)
{
    QTimer *timer = new QTimer(loop);
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, loop, &QEventLoop::quit);
    timer->start(msecs);
    return loop->exec();
}

QString util::getHtmlIconTextVertical(const QString &icon, const QString &text)
{
    return QStringLiteral("<div><img src=\"%1\"</img><div style=\"margin-top: 5px;\">%2</div></div>").arg(icon).arg(text);
}

#if 1
bool util::setProgramUnique(const QString &name)
{
    QLocalSocket localSocket;
    localSocket.connectToServer(name, QIODevice::WriteOnly);
    if(localSocket.waitForConnected())
    {
        char kk[1] = {'\0'};
        localSocket.write(kk, 3);
        localSocket.close();

        qWarning() << name << "is Existed.";
        return false;
    }
    localSocket.close();

    QLocalServer::removeServer(name);
    QLocalServer *localServer = new QLocalServer(qApp);
    localServer->listen(name);
    return true;
}
#endif

QString util::checkFile(const QString &filepath, bool isCover)
{
    QFileInfo finfo(filepath);
    QString suffix = finfo.completeSuffix();
    QString name = finfo.baseName();
    QString path = finfo.absolutePath();
    QDir dir;
    dir.mkpath(path);

    QString url = filepath;
    QFile file(url);
    if (isCover)
    {
        if (file.exists())
        {
            file.remove();
        }
    }
    else
    {
        int i = 0;
        while(file.exists())
        {
            ++i;
            url = QString("%1/%2_%3.%4").arg(path).arg(name).arg(i).arg(suffix);
            file.setFileName(url);
        }
    }
    return url;
}

bool util::rmDir(const QString &path)
{
    if (path.isEmpty())
        return false;

    QDir dir(path);
    if(!dir.exists())
        return true;

    return dir.removeRecursively();
}

QString util::Json2String(const QJsonObject &json)
{
    QJsonDocument doc;
    doc.setObject(json);
    return doc.toJson(QJsonDocument::Compact);
}

QJsonObject util::String2Json(const QString &data, QString *err)
{
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError)
    {
        if (err) *err = parseErr.errorString();
    }
    return doc.object();
}

qint64 util::mSecsSinceEpoch()
{
    return QDateTime::currentMSecsSinceEpoch();
}

bool util::ensureDirExist(const QString &dirPath)
{
    QDir dir(dirPath);
    if (dir.exists())
    {
        return true;
    }
    return dir.mkpath(dirPath);
}

int util::screenWidth()
{
    return QGuiApplication::primaryScreen()->geometry().width();
}

int util::screenHeight()
{
    return QGuiApplication::primaryScreen()->geometry().height();
}

int util::scaleWidthByResolution(int width)
{
    int screenWidth = QGuiApplication::primaryScreen()->geometry().width();
    int newWidth = width / (float)1920 * screenWidth;
    return newWidth;
}

QSize util::newSize(QSize size)
{
    int screenWidth = QGuiApplication::primaryScreen()->geometry().width();
    int screenHeight = QGuiApplication::primaryScreen()->geometry().height();

    int newWidth = size.width() / (float)1920 * screenWidth;
    int newHeight = size.height()/ (float)1080 * screenHeight;
    return QSize(newWidth, newHeight);
}

#if 0
QString util::localIpv4()
{
    QList<QNetworkInterface> interfaceList = QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface interfaceItem, interfaceList)
    {
        if (interfaceItem.flags().testFlag(QNetworkInterface::IsUp)
            && interfaceItem.flags().testFlag(QNetworkInterface::IsRunning)
            && interfaceItem.flags().testFlag(QNetworkInterface::CanBroadcast)
            && interfaceItem.flags().testFlag(QNetworkInterface::CanMulticast)
            && !interfaceItem.flags().testFlag(QNetworkInterface::IsLoopBack)
            && interfaceItem.hardwareAddress() != "00:50:56:C0:00:01"
            &&interfaceItem.hardwareAddress() != "00:50:56:C0:00:08")
        {
            QList<QNetworkAddressEntry> addressEntryList = interfaceItem.addressEntries();
            foreach(QNetworkAddressEntry addressEntryItem, addressEntryList)
            {
                if (addressEntryItem.ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    return addressEntryItem.ip().toString();
                }
            }
        }
    }
    return QString("");
}
#endif

QString util::formatTime(int secs, const QString &format)
{
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = (secs % 3600) % 60;

    return format.arg(h).arg(m).arg(s);
}

QStringList util::secToTime(int secs)
{
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = (secs % 3600) % 60;

    QString hh = QString("%1").arg(h, 2, 10, QChar('0'));
    QString mh = QString("%1").arg(m, 2, 10, QChar('0'));
    QString sh = QString("%1").arg(s, 2, 10, QChar('0'));
    QStringList sl;
    sl.append(hh);
    sl.append(mh);
    sl.append(sh);
    
    return sl;
}

QString util::timestamp2String(qint64 ms)
{
    return QDateTime::fromMSecsSinceEpoch(ms).toString("yyyy-MM-dd hh:mm:ss.zzz");
}

QString util::QVariant2QString(const QVariant &var)
{
    return QJsonDocument::fromVariant(var).toJson(QJsonDocument::Compact);
}

QVariant util::QString2QVariant(const QString &s)
{
    return QJsonDocument::fromJson(s.toUtf8(), nullptr).toVariant();
}

bool util::isDriveExist(const QString &drive)
{
#ifdef WIN32
	for (const auto& fi : QDir::drives())
	{
		QString path = fi.filePath();
		UINT ret = GetDriveType((WCHAR*)path.utf16());
		if (path == drive && ret == DRIVE_FIXED)
		{
			return true;
		}
	}

	//qWarning() << qstr("驱动器%1不存在，或者不是硬盘").arg(drive);
	qWarning() << QString("驱动器%1不存在，或者不是硬盘").arg(drive);
	return false;
#else
    // 暂时不判断
    return true;
#endif // WIN32

}

bool util::isFileExist(const QString &path)
{
    QFile file(path);
    return file.exists();
}

QStringList util::filePathListInDir(const QString &dirPath, QStringList filter)
{
    QDir dir(dirPath);
    if (!dir.exists())
    {
        return QStringList();
    }
    QStringList pathList;
    QFileInfoList list = dir.entryInfoList(filter, QDir::Files | QDir::Readable, QDir::Name);
    foreach (const QFileInfo& fi , list)
    {
        pathList.append(fi.absoluteFilePath());
    }
    return pathList;
}



QString util::appDirPath()
{
    return QCoreApplication::applicationDirPath();
}

bool util::isDirExist(const QString &path)
{
    QDir dir(path);
    return dir.exists();
}

QString util::getExecutableExePath(const QString &exePath)
{
    // 避免路径带空格启动失败（例如：C:\Program Files (x86)\pcbt）
    return QString("\"" + exePath + "\"");
}

void util::setRetainSizeWhenHidden(QWidget* w, bool isRetain)
{
    QSizePolicy policy = w->sizePolicy();
    policy.setRetainSizeWhenHidden(isRetain);
    w->setSizePolicy(policy);
}

bool util::removeFile(const QString& dirPath, const QString& nameFilter)
{
    if (dirPath.isEmpty())
        return false;

    QDir dir(dirPath, nameFilter, QDir::Name | QDir::IgnoreCase, QDir::Files | QDir::NoSymLinks);
    if (dir.exists())
    {
        QFileInfoList fileList = dir.entryInfoList();
        for (const auto fi : fileList) 
        {
            if (fi.isFile())
            {
                fi.dir().remove(fi.fileName());
            }
        }
    }
    return true;
}

QString util::getAppName(const QString& argv0)
{
	QString appName = QCoreApplication::applicationName();
	if (appName.isEmpty())
	{
		QString appPath = argv0;
		QStringList l = appPath.split('\\');
		if (l.length() <= 1)
		{
			return appName;
		}
		QString exe = l.last();
		QStringList exeList = exe.split('.');
		if (exeList.length() != 2)
		{
			return appName;
		}
		appName = exeList[0];
        QCoreApplication::setApplicationName(appName);
	}
    return appName;
}

bool util::isDiskSpaceEnough(QString dir)
{
    if (dir.isEmpty())
    {
        dir = QStandardPaths::standardLocations(QStandardPaths::DesktopLocation)[0].append("/");
    }

    QStorageInfo si(dir);
    float available = (float)si.bytesAvailable() / 1024 / 1024 / 1024;
    float total = (float)si.bytesTotal() / 1024 / 1024 / 1024;
    float percent = available / total;
    qInfo() << /*QStringLiteral*/QString("目录%1所在磁盘的可用空间为%2%").arg(dir).arg((int)(percent * 100));
#if 1
    if (percent <= 0.1)
    {
        //m_recordEnabled = false;
        //CMessageBox::info(qstr("%1盘空间不足!\n清理磁盘或修改录制视频保存路径").arg(m_recordPath.left(1)));
        return false;
    }
#endif
    return true;
}
