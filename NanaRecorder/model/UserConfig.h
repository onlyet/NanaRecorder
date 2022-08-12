#pragma once

#include <CSetting.h>

/*! @brief 配置键 */
namespace CK {

namespace User {
constexpr char Account[] = "User/Account";
constexpr char Pwd[] = "User/Pwd";
constexpr char RememberPwd[] = "User/RememberPwd";

}

} // namespace CK

class UserConfig : public CSetting
{
private:
    UserConfig();
    ~UserConfig() override;

#if (QT_VERSION <= QT_VERSION_CHECK(5,15,0))
		Q_DISABLE_COPY(UserConfig)
			UserConfig(UserConfig&&) = delete;
		UserConfig& operator=(UserConfig&&) = delete;
#else
		Q_DISABLE_COPY_MOVE(UserConfig)
#endif

public:
    static UserConfig *instance();

protected:
    void regist() override;
};

#define USERCFG UserConfig::instance()
