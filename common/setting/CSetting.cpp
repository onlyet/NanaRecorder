#include "CSetting.h"

#include <QDebug>
#include <QRegExpValidator>
#include <QSettings>
#include <QTextCodec>

CSetting::CSetting() = default;
CSetting::~CSetting() = default;

void CSetting::init(const QString &filepath)
{
    if (!m_setting.isNull())
        return;

    m_setting.reset(new QSettings(filepath, QSettings::IniFormat));
    m_setting->setIniCodec(QTextCodec::codecForName("utf-8"));

    qDebug() << "加载配置文件" << filepath;

    regist();

    // 保存已加载的配置
    for (auto itor = m_values.constBegin(); itor != m_values.constEnd(); ++itor)
    {
        m_setting->setValue(itor.key(), itor.value());
    }

    qDebug() << "配置加载完成";
}

QVariant CSetting::get(const QString &key, const QVariant &defaultValue)
{
    return m_setting->value(key, defaultValue);
}

void CSetting::set(const QString &key, const QVariant &value)
{
    m_values.insert(key, value);
    m_setting->setValue(key, value);
}

bool CSetting::getBool(const QString &key, int defaultValue)
{
    return get(key, defaultValue).toInt();
}

QString CSetting::getStr(const QString &key, const QString &defaultValue)
{
    return get(key, defaultValue).toString();
}

int CSetting::getInt(const QString &key, int defaultValue)
{
    return get(key, defaultValue).toInt();
}

void CSetting::load(const QString &key, const QVariant &defaultValue, CSetting::FuncIsValid funcIsValid)
{
    QVariant v = m_setting->value(key, defaultValue);
    if (funcIsValid && !funcIsValid(v))
    {
        qDebug().nospace() << "Invalid Setting. Key:" << key << ", Value:" << v;
        v = defaultValue;
    }
    m_values.insert(key, v);
}

bool CSetting::isValid_Bool(const QVariant &v)
{
    bool flag = false;
    const auto n = v.toInt(&flag);
    return flag && (n == TRUE || n == FALSE);
}

bool CSetting::isValid_String(const QVariant &v)
{
    return !v.toString().isEmpty();
}

CSetting::FuncIsValid CSetting::isValid_Reg(const QString &reg)
{
    static auto func = [](const QString &reg , const QVariant &v) {
        QRegExp e(reg);
        QRegExpValidator val(e);
        int pos = 0;
        auto str = v.toString();
        return QValidator::Acceptable == val.validate(str, pos);
    };
    return std::bind(func, reg, std::placeholders::_1);
}

CSetting::FuncIsValid CSetting::isValid_Int_Greater(int n)
{
    static auto func = [](int n , const QVariant &v) {
        bool flag = false;
        return v.toInt(&flag) > n && flag;
    };
    return std::bind(func, n, std::placeholders::_1);
}

CSetting::FuncIsValid CSetting::isValid_Int_Less(int n)
{
    static auto func = [](int n, const QVariant &v) {
        bool flag = false;
        return v.toInt(&flag) < n && flag;
    };
    return std::bind(func, n, std::placeholders::_1);
}

CSetting::FuncIsValid CSetting::isValid_Int_Between(int a, int b)
{
    static auto func = [](int a, int b, const QVariant &v) {
        bool flag = false;
        const auto n = v.toInt(&flag);
        return flag && n >= a && n <= b;
    };
    return std::bind(func, a, b, std::placeholders::_1);
}
