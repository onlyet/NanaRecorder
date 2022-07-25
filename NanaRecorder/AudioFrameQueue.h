#pragma once
#include "FFmpegHeader.h"
#include "RecordConfig.h"

#include <condition_variable>


class AudioFrameQueue {
public:
    int      initBuf(AVCodecContext* encodeCtx);
    void     deinit();
    int      writeFrame(AVFrame* frame);
    AVFrame* readFrame();

private:
    bool                    m_isInit   = false;
    AVAudioFifo*            m_aFifoBuf = nullptr;
    std::mutex              m_mtxVBuf;
    std::condition_variable m_cvABufNotFull;
    std::condition_variable m_cvABufNotEmpty;
    int                     m_vFrameSize;
    //AVFrame*                m_vInFrame     = nullptr;
    //uint8_t*                m_vInFrameBuf  = nullptr;
    AVFrame*                m_aOutFrame    = nullptr;
    uint8_t*                m_aOutFrameBuf = nullptr;
    SwrContext*             m_swrCtx       = nullptr;
    //VideoCaptureInfo        m_videoCapInfo;
};
