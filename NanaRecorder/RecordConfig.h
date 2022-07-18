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
    std::condition_variable cvNotPause;   // �������ͣ��ʱ�������ɼ��̹߳���
    std::mutex              mtxPause;

    //std::condition_variable     cvVBufNotFull;
    //std::condition_variable     cvVBufNotEmpty;
    //std::mutex                  mtxVBuf;
    //AVFifoBuffer*               vFifoBuf;
    //int                         vOutFrameItemSize;
};

#define g_record Singleton<RecordConfig>::instance()