#include "VideoFrameQueue.h"

#include <QDebug>

#include <mutex>

using namespace std;

int VideoFrameQueue::initBuf(int width, int height, int format)
{
    m_width = width;
    m_height = height;
    m_format = format;

    enum AVPixelFormat fmt = static_cast<enum AVPixelFormat>(format);
    m_vFrameSize = av_image_get_buffer_size(fmt, width, height, 1);

    m_vInFrameBuf = (uint8_t*)av_malloc(m_vFrameSize);
    m_vInFrame = av_frame_alloc();
    m_vInFrame->width = width;
    m_vInFrame->height = height;
    m_vInFrame->format = format;
    av_image_fill_arrays(m_vInFrame->data, m_vInFrame->linesize, m_vInFrameBuf,
        fmt, width, height, 1);

    m_vOutFrame = av_frame_alloc();
    m_vOutFrame->width = width;
    m_vOutFrame->height = height;
    m_vOutFrame->format = format;
    m_vOutFrameBuf = (uint8_t*)av_malloc(m_vFrameSize);
    av_image_fill_arrays(m_vOutFrame->data, m_vOutFrame->linesize, m_vOutFrameBuf,
        fmt, width, height, 1);

    //申请30帧缓存
    m_vFrameItemSize = m_vFrameSize + sizeof(int64_t);
    m_vFifoBuf = av_fifo_alloc(30 * m_vFrameItemSize);
    if (!m_vFifoBuf) {
        qDebug() << "av_fifo_alloc_array failed";
        return -1;
    }

    return 0;
}

int VideoFrameQueue::writeFrame(AVFrame* oldFrame, AVCodecContext* m_vDecodeCtx, int64_t captureTime)
{
    enum AVPixelFormat fmt = static_cast<enum AVPixelFormat>(m_format);
    if (m_vDecodeCtx->width != m_width || m_vDecodeCtx->height != m_height
        || m_vDecodeCtx->pix_fmt != fmt) {
        m_swsCtx = sws_getContext(m_vDecodeCtx->width, m_vDecodeCtx->height, m_vDecodeCtx->pix_fmt,
            m_width, m_height, fmt,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    }
    // srcSliceH应该是输入高度，return输出高度
    sws_scale(m_swsCtx, (const uint8_t* const*)oldFrame->data, oldFrame->linesize, 0,
        m_vDecodeCtx->height, m_vInFrame->data, m_vInFrame->linesize);

    {
        unique_lock<mutex> lk(m_mtxVBuf);
        m_cvVBufNotFull.wait(lk, [this] { return av_fifo_space(m_vFifoBuf) >= m_vFrameItemSize; });
    }

    av_fifo_generic_write(m_vFifoBuf, &captureTime, sizeof(int64_t), NULL);
    av_fifo_generic_write(m_vFifoBuf, &m_vInFrameBuf, m_vFrameSize, NULL);
    m_cvVBufNotEmpty.notify_one();

    return 0;
}

int VideoFrameQueue::readFrame()
{
    return 0;
}
