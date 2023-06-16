#ifndef ONLYET_RECORDCONFIG_H
#define ONLYET_RECORDCONFIG_H

#include "singleton.h"

#include <QString>

#include <condition_variable>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>

#ifdef __cplusplus
};
#endif

namespace onlyet {

enum RecordStatus {
    Stopped = 0,
    Running,
    Paused,
};

enum class AudioCaptureDevice {
    Speaker = 0,  // ������
    Microphone    // ��˷�
};

enum class AudioCaptureType {
    OnlySpeaker = 0,
    OnlyMicrophone,
    SpeakerAndMicrophone
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

    bool             enableAudio;
    AudioCaptureType audioCaptureType;
    int              channel;
    int              sampleRate;

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

}  // namespace onlyet

#endif  // !ONLYET_RECORDCONFIG_H
