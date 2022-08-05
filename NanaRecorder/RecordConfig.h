#pragma once

#include "singleton.h"
#include "FFmpegHeader.h"

#include <QString>

#include <condition_variable>

enum RecordStatus {
    Stopped = 0,
    Running,
    Paused,
};

struct VideoCaptureInfo {
    int width;
    int height;
    AVPixelFormat format;
};

struct AudioCaptureInfo {
    int64_t        channelLayout;
    AVSampleFormat format;
    int            sampleRate;
};

struct RecordConfig {
    friend Singleton<RecordConfig>;

    QString     filePath;
    int         width;
    int         height;
    int         fps;
    int         audioBitrate;

    RecordStatus status = Stopped;
    std::condition_variable cvNotPause;   // 当点击暂停的时候，两个采集线程挂起
    std::mutex              mtxPause;
};

#define g_record Singleton<RecordConfig>::instance()