#pragma once
#include "FFmpegHeader.h"

#include <condition_variable>


class VideoFrameQueue
{
public:
    int initBuf(int width, int height, int format);
    int writeFrame(AVFrame* frame, AVCodecContext* m_vDecodeCtx, int64_t captureTime);
    int readFrame();

private:
    int m_width;
    int m_height;
    int m_format;

    std::condition_variable     m_cvVBufNotFull;
    std::condition_variable     m_cvVBufNotEmpty;
    std::mutex                  m_mtxVBuf;
    AVFifoBuffer*               m_vFifoBuf = nullptr;
    //int                         m_vOutFrameItemSize;

    int                         m_vFrameSize; // 一帧YUV数据字节数
    int                         m_vFrameItemSize; // m_vFrameSize + sizeof(int64_t)
    AVFrame*                    m_vInFrame = nullptr;
    uint8_t*                    m_vInFrameBuf = nullptr;
    AVFrame*                    m_vOutFrame = nullptr;
    uint8_t*                    m_vOutFrameBuf = nullptr;

    SwsContext*                 m_swsCtx = nullptr;

};

