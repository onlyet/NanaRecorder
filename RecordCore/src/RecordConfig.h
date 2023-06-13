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
    int           width;  // ������
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

    int inWidth;  // ������
    int inHeight;

    bool enableAudio;
    int  audioDeviceIndex;  // 0����������1����˷�
    int  channel;
    int  sampleRate;

    QString filePath;  // ¼���ļ�����·��
    int     outWidth;  // ������
    int     outHeight;
    int     fps;
    int     audioBitrate;

    RecordStatus            status = Stopped;
    std::condition_variable cvNotPause;  // �������ͣ��ʱ�������ɼ��̹߳���
    std::mutex              mtxPause;
};

#define g_record Singleton<RecordConfig>::instance()