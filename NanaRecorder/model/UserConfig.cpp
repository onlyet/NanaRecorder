#include "UserConfig.h"

UserConfig::UserConfig()=default;
UserConfig::~UserConfig()=default;

UserConfig *UserConfig::instance()
{
    static UserConfig inst;
    return &inst;
}

void UserConfig::regist()
{
    //load(CK::Exam::ExamRoom, "");
}
