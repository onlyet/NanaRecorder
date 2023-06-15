#ifndef ONLYET_AUDIOCAPTURE_H
#define ONLYET_AUDIOCAPTURE_H

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

namespace onlyet {

class AudioCaptureInfo;
enum class AudioCaptureDevice;

class AudioCapture {
    using AmixFilterCb = std::function<void(AVFrame*, int)>;

public:
    int startCapture(AudioCaptureDevice dev);
    int stopCapture();

    void setAmixFilterCb(AmixFilterCb cb, int filterCtxIdx) {
        m_amixFilterCb   = cb;
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

    void audioCaptureThread();

private:
    std::atomic_bool m_isRunning  = false;
    int              m_aIndex     = -1;  // ÊäÈëÒôÆµÁ÷Ë÷Òý
    AVFormatContext* m_aFmtCtx    = nullptr;
    AVCodecContext*  m_aDecodeCtx = nullptr;
    std::thread      m_captureThread;
    AmixFilterCb     m_amixFilterCb;
    int              m_filterCtxIndex;
};

}  // namespace onlyet
#endif  // !ONLYET_AUDIOCAPTURE_H
