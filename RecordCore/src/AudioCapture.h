#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>

#ifdef __cplusplus
};
#endif

#include <atomic>
#include <functional>
#include <thread>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;

class AudioCaptureInfo;
enum class AudioCaptureDevice;

class AudioCapture {
    using AudioFrameCb = std::function<void(AVFrame*, int)>;
public:
    int startCapture(AudioCaptureDevice dev);
    int stopCapture();

    void setFrameCb(AudioFrameCb cb, int filterCtxIdx) {
        m_frameCb    = cb;
        m_filterCtxIndex = filterCtxIdx;
    }

    AVRational     timebase();
    int            sampleRate();
    AVSampleFormat sampleFormat();
    int            channel();
    int64_t        channelLayout();

private:
    int  initCapture(AudioCaptureDevice dev);
    void deinit();

    void audioCaptureThreadProc();

private:
    std::atomic_bool                                       m_isRunning  = false;
    int                                                    m_aIndex     = -1;  // ÊäÈëÒôÆµÁ÷Ë÷Òý
    AVFormatContext*                                       m_aFmtCtx    = nullptr;
    AVCodecContext*                                        m_aDecodeCtx = nullptr;
    std::thread                                            m_captureThread;
    AudioFrameCb                                           m_frameCb;
    int                                                    m_filterCtxIndex;
};
