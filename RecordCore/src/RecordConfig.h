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
    Speaker = 0,  // Speaker
    Microphone    // microphone
};

enum class AudioCaptureType {
    OnlySpeaker = 0,
    OnlyMicrophone,
    SpeakerAndMicrophone
};

struct VideoCaptureInfo {
    int           width;  // Input width and height
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

    int inWidth;  // Input width and height
    int inHeight;

    bool             enableAudio;
    AudioCaptureType audioCaptureType;
    int              channel;
    int              sampleRate;

    QString filePath;  // Path for saving recorded files
    int     outWidth;  // Output width and height
    int     outHeight;
    int     fps;
    int     audioBitrate;

    RecordStatus            status = Stopped;
    std::condition_variable cvNotPause;  // When pause is clicked, both acquisition threads are suspended.
    std::mutex              mtxPause;
};

#define g_record Singleton<RecordConfig>::instance()

}  // namespace onlyet

#endif  // !ONLYET_RECORDCONFIG_H
