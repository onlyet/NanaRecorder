#pragma once
//#include "RecordConfig.h"

#include "FFmpegHeader.h"

#include <thread>


class VideoCapture
{
public:
    //VideoCapture(const RecordInfo& info);
    int startCapture();
    int stopCapture();

private:
    int initCapture();

    void captureThreadProc();

private:
    int              m_vIndex;       // 输入视频流索引
    AVFormatContext* m_vFmtCtx = nullptr;
    AVCodecContext* m_vDecodeCtx = nullptr;
    SwsContext*     m_swsCtx = nullptr;

    std::thread     m_captureThread;
};

