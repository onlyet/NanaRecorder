#pragma once
#include "FFmpegHeader.h"

#include <atomic>
#include <functional>
#include <thread>

class AudioCaptureInfo;

class AudioCapture {
public:
    int startCapture();
    int stopCapture();

    void setFrameCb(std::function<void(AVFrame*, const AudioCaptureInfo&)> cb) {
        m_frameCb = cb;
    }

private:
    int initCapture();
    void deinit();

    void AudioCaptureThreadProc();

private:
    std::atomic_bool m_isRunning = false;
    int m_vIndex = -1;  // 输入视频流索引
    AVFormatContext* m_vFmtCtx = nullptr;
    AVCodecContext* m_vDecodeCtx = nullptr;
    std::thread m_captureThread;
    std::function<void(AVFrame*, const AudioCaptureInfo&)> m_frameCb;
};
