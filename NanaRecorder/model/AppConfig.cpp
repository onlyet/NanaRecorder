#include "AppConfig.h"

AppConfig::AppConfig() = default;
AppConfig::~AppConfig() = default;

AppConfig *AppConfig::instance()
{
    static AppConfig inst;
    return &inst;
}

void AppConfig::regist()
{
    load(CK::Server::Address, "XXX", isValid_String);
}
