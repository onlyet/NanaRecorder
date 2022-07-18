#pragma once
//#include "RecordConfig.h"

#include "FFmpegHeader.h"

#include <thread>
#include <functional>

class VideoCapture
{
public:
    //VideoCapture(const RecordInfo& info);
    int startCapture();
    int stopCapture();

    void setFrameCb(std::function<void(AVFrame*, AVCodecContext*)> cb) { m_frameCb = cb; }

private:
    int initCapture();

    void captureThreadProc();

private:
    int              m_vIndex;       // 输入视频流索引
    AVFormatContext* m_vFmtCtx = nullptr;
    AVCodecContext* m_vDecodeCtx = nullptr;
    //SwsContext*     m_swsCtx = nullptr;

    std::thread     m_captureThread;

    std::function<void(AVFrame*, AVCodecContext*)> m_frameCb;
};

