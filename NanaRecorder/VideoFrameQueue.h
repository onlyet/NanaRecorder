#pragma once
#include "FFmpegHeader.h"

#include <condition_variable>


struct FrameItem {
    int64_t captureTime;
    AVFrame* frame = nullptr;
};

class VideoFrameQueue
{
public:
    int initBuf(int width, int height, AVPixelFormat format);
    int writeFrame(AVFrame* frame, AVCodecContext* m_vDecodeCtx, int64_t captureTime);
    FrameItem* readFrame();

private:
    int m_width;
    int m_height;
    //int m_format;
    AVPixelFormat m_format;
    AVFifoBuffer*               m_vFifoBuf = nullptr;
    std::mutex                  m_mtxVBuf;
    std::condition_variable     m_cvVBufNotFull;
    std::condition_variable     m_cvVBufNotEmpty;

    int                         m_vFrameSize;           // 一帧YUV数据字节数
    int                         m_vFrameItemSize;       // m_vFrameSize + sizeof(int64_t)
    AVFrame*                    m_vInFrame = nullptr;
    uint8_t*                    m_vInFrameBuf = nullptr;
    AVFrame*                    m_vOutFrame = nullptr;
    uint8_t*                    m_vOutFrameBuf = nullptr;
    FrameItem                   m_vFrameItem;

    SwsContext*                 m_swsCtx = nullptr;

};

