#pragma once

#include <CSetting.h>

/*! @brief 配置键 */
namespace CK {

    namespace Volatile {
    } // namespace Volatile

    namespace Local {
        constexpr char EnableLogo[] = "Server/EnableLogo";
    } // namespace Local

    namespace Server {
        constexpr char EnableServer[] = "Server/EnableServer";
        constexpr char Address[] = "Server/ServerAddress";         // 服务器地址
    } // namespace Server

} // namespace CK

class AppConfig : public CSetting
{
private:
    AppConfig();
    ~AppConfig() override;

#if (QT_VERSION <= QT_VERSION_CHECK(5,15,0))
    Q_DISABLE_COPY(AppConfig)
        AppConfig(AppConfig&&) = delete;
    AppConfig& operator=(AppConfig&&) = delete;
#else
    Q_DISABLE_COPY_MOVE(AppConfig)
#endif

public:
    static AppConfig* instance();

protected:
    void regist() override;
};

#define APPCFG AppConfig::instance()
