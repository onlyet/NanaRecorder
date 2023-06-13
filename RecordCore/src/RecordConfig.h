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
    int           width;  // 输入宽高
    int           height;
    AVPixelFormat format;
};

struct AudioCaptureInfo {
    int64_t        channelLayout;
    AVSampleFormat format;
    int            sampleRate;
};

struct RecordConfig {
    friend Singleton<RecordConfig>;

    int inWidth;  // 输入宽高
    int inHeight;

    bool enableAudio;
    int  audioDeviceIndex;  // 0：扬声器，1：麦克风
    int  channel;
    int  sampleRate;

    QString filePath;  // 录制文件保存路径
    int     outWidth;  // 输出宽高
    int     outHeight;
    int     fps;
    int     audioBitrate;

    RecordStatus            status = Stopped;
    std::condition_variable cvNotPause;  // 当点击暂停的时候，两个采集线程挂起
    std::mutex              mtxPause;
};

#define g_record Singleton<RecordConfig>::instance()