#ifndef CSETTING_H
#define CSETTING_H

#include <QHash>
#include <QMutex>
#include <QVariant>

#ifndef TRUE
#  define TRUE 1
#endif

#ifndef FALSE
#  define FALSE 0
#endif

class QSettings;

/**
 * @brief 配置类
 * @note 增删是非线程安全的
 *       改查是线程安全的
 */
class CSetting
{
public:
    CSetting();
    virtual ~CSetting();

    void init(const QString &filepath);

    QVariant get(const QString &key, const QVariant &defaultValue = QVariant());
    void set(const QString &key, const QVariant &value);

    bool getBool(const QString &key, int defaultValue = FALSE);
    QString getStr(const QString &key, const QString &defaultValue = QString());
    int getInt(const QString &key, int defaultValue = 0);

protected:
    /*! @brief 注册所有配置 */
    virtual void regist() = 0;

    using FuncIsValid = std::function<bool (const QVariant &v)>;
    void load(const QString &key, const QVariant &defaultValue = QVariant(), FuncIsValid funcIsValid = nullptr);

    /*! @brief 校验函数, 当配置项是0或1时返回true */
    static bool isValid_Bool(const QVariant &v);
    /*! @brief 校验函数, 当配置项是非空字符串时返回true */
    static bool isValid_String(const QVariant &v);
    /*! @brief 校验函数, 高阶函数, 当配置项满足正则表达式时返回true */
    static FuncIsValid isValid_Reg(const QString &reg);
    /*! @brief 校验函数, 高阶函数, 当配置项 >n 时返回true */
    static FuncIsValid isValid_Int_Greater(int n);
    /*! @brief 校验函数, 高阶函数, 当配置项 <n 时返回true */
    static FuncIsValid isValid_Int_Less(int n);
    /*! @brief 校验函数, 高阶函数, 当配置项 >=a 且 <=b 时返回true */
    static FuncIsValid isValid_Int_Between(int a, int b);

protected:
    QScopedPointer<QSettings> m_setting;
    QHash<QString, QVariant> m_values;
};

#endif // CSETTING_H
