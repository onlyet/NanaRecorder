#ifndef ONLYET_VIDEOCAPTURE_H
#define ONLYET_VIDEOCAPTURE_H

#include <thread>
#include <functional>
#include <atomic>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;

namespace onlyet {

class VideoCaptureInfo;

class VideoCapture {
public:
    int startCapture();
    int stopCapture();

    void setFrameCb(std::function<void(AVFrame*, const VideoCaptureInfo&)> cb) { m_frameCb = cb; }

private:
    int  initCapture();
    void deinit();

    void videoCaptureThreadProc();

private:
    std::atomic_bool                                       m_isRunning  = false;
    int                                                    m_vIndex     = -1;  // 输入视频流索引
    AVFormatContext*                                       m_vFmtCtx    = nullptr;
    AVCodecContext*                                        m_vDecodeCtx = nullptr;
    std::thread                                            m_captureThread;
    std::function<void(AVFrame*, const VideoCaptureInfo&)> m_frameCb;
};

}  // namespace onlyet

#endif  // !ONLYET_VIDEOCAPTURE_H
