#ifndef ONLYET_RECORDER_H
#define ONLYET_RECORDER_H

#include "IRecorder.h"

#include <QVariant>

#include <chrono>
#include <memory>

class AVFrame;
class AVCodecContext;

namespace onlyet {

class VideoCapture;
class VideoFrameQueue;
class FrameItem;
class VideoCaptureInfo;
class AudioCapture;
class AudioFrameQueue;
class AudioCaptureInfo;
class FileOutputer;

class AmixFilter;
class ResampleFilter;

template <typename clock_type>
class Timer;

class Recorder : public IRecorder {
public:
    Recorder(const QVariantMap& recordInfo);
    ~Recorder();

    void setRecordInfo(const QVariantMap& recordInfo) override;
    int  startRecord() override;
    int  pauseRecord() override;
    int  resumeRecord() override;
    int  stopRecord() override;

private:
    int startCapture();
    void stopCapture();

    void       writeVideoFrameCb(AVFrame* frame, const VideoCaptureInfo& info);
    FrameItem* readVideoFrameCb();

    void     initAudioBufCb(AVCodecContext* encodeCtx);
    void     writeAudioFrameCb(AVFrame* frame, const AudioCaptureInfo& info);
    AVFrame* readAudioFrameCb();

    int64_t getPauseDurationCb() { return m_pauseDuration; }

    void addFrameToAmixFilter(AVFrame* frame, int filterCtxIndex);
    void writeAudioFrameCb(AVFrame* frame);

private:
    VideoCapture*                                     m_videoCap        = nullptr;
    VideoFrameQueue*                                  m_videoFrameQueue = nullptr;
    AudioCapture*                                     m_speakerCap      = nullptr;
    AudioCapture*                                     m_microphoneCap   = nullptr;
    AudioFrameQueue*                                  m_audioFrameQueue = nullptr;
    FileOutputer*                                     m_outputer        = nullptr;
    int64_t                                           m_startTime       = -1;       // 录制开始时间戳（微秒）
    int64_t                                           m_pauseDuration   = 0;        // 暂停持续时间
    std::unique_ptr<Timer<std::chrono::system_clock>> m_pauseStopwatch  = nullptr;  // 暂停秒表

    AmixFilter*     m_amixFilter     = nullptr;
    ResampleFilter* m_resampleFilter = nullptr;
};

}  // namespace onlyet

#endif  // !ONLYET_RECORDER_H