#pragma once

#include "singleton.h"

#include <QString>

#include <condition_variable>

enum RecordStatus {
    Stopped,
    Started,
    Paused,
};

struct RecordConfig {
    friend Singleton<RecordConfig>;

    QString     filePath;
    int         width;
    int         height;
    int         fps;
    int         audioBitrate;

    RecordStatus status;
    std::condition_variable cvNotPause;   // 当点击暂停的时候，两个采集线程挂起
    std::mutex              mtxPause;

    //std::condition_variable     cvVBufNotFull;
    //std::condition_variable     cvVBufNotEmpty;
    //std::mutex                  mtxVBuf;
    //AVFifoBuffer*               vFifoBuf;
    //int                         vOutFrameItemSize;
};

#define g_record Singleton<RecordConfig>::instance()