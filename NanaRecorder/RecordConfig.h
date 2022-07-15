#pragma once

#include "singleton.h"

#include <QString>

struct RecordConfig {
    friend Singleton<RecordConfig>;

    QString     filePath;
    int         width;
    int         height;
    int         fps;
    int         audioBitrate;
};

#define g_record Singleton<RecordConfig>::instance()