#ifndef ONLYET_VIDEOFRAMEQUEUE_H
#define ONLYET_VIDEOFRAMEQUEUE_H

#include "FFmpegHeader.h"
#include "RecordConfig.h"

#include <condition_variable>

namespace onlyet {

struct FrameItem {
    int64_t  captureTime;
    AVFrame* frame = nullptr;
};

class VideoFrameQueue {
public:
    int        initBuf(int width, int height, AVPixelFormat format);
    void       deinit();
    int        writeFrame(AVFrame* frame, const VideoCaptureInfo& info, int64_t captureTime);
    FrameItem* readFrame();

private:
    bool                    m_isInit = false;
    int                     m_width;  // 输出宽高
    int                     m_height;
    AVPixelFormat           m_format;
    AVFifoBuffer*           m_vFifoBuf = nullptr;
    std::mutex              m_mtxVBuf;
    std::condition_variable m_cvVBufNotFull;
    std::condition_variable m_cvVBufNotEmpty;
    int                     m_vFrameSize;      // 一帧YUV数据字节数
    int                     m_vFrameItemSize;  // m_vFrameSize + sizeof(int64_t)
    AVFrame*                m_vInFrame     = nullptr;
    uint8_t*                m_vInFrameBuf  = nullptr;
    AVFrame*                m_vOutFrame    = nullptr;
    uint8_t*                m_vOutFrameBuf = nullptr;
    FrameItem               m_vFrameItem;
    SwsContext*             m_swsCtx = nullptr;
    VideoCaptureInfo        m_videoCapInfo;  // 解码获取到的视频信息
    bool                    m_needScale = false;
};

}  // namespace onlyet

#endif  // !ONLYET_VIDEOFRAMEQUEUE_H
