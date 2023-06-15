#include "VideoFrameQueue.h"
#include "RecordConfig.h"

#include <QDebug>
#include <QTime>
#include <QDateTime>

#include <mutex>

using namespace std;

namespace onlyet {

int VideoFrameQueue::initBuf(int width, int height, AVPixelFormat format) {
    m_width  = width;
    m_height = height;
    m_format = format;

    m_vFrameSize = av_image_get_buffer_size(format, width, height, 1);

    m_vInFrameBuf      = (uint8_t*)av_malloc(m_vFrameSize);
    m_vInFrame         = av_frame_alloc();
    m_vInFrame->width  = width;
    m_vInFrame->height = height;
    m_vInFrame->format = format;
    av_image_fill_arrays(m_vInFrame->data, m_vInFrame->linesize, m_vInFrameBuf,
                         format, width, height, 1);

    m_vOutFrame         = av_frame_alloc();
    m_vOutFrame->width  = width;
    m_vOutFrame->height = height;
    m_vOutFrame->format = format;
    m_vOutFrameBuf      = (uint8_t*)av_malloc(m_vFrameSize);
    av_image_fill_arrays(m_vOutFrame->data, m_vOutFrame->linesize, m_vOutFrameBuf,
                         format, width, height, 1);

    //申请30帧缓存
    m_vFrameItemSize = m_vFrameSize + sizeof(int64_t);
    m_vFifoBuf       = av_fifo_alloc(30 * m_vFrameItemSize);
    if (!m_vFifoBuf) {
        qCritical() << "av_fifo_alloc_array failed";
        return -1;
    }

    m_isInit = true;
    return 0;
}

void VideoFrameQueue::deinit() {
    m_isInit = false;

    if (m_vFifoBuf) {
        av_fifo_freep(&m_vFifoBuf);
        m_vFifoBuf = nullptr;
    }
    if (m_vInFrame) {
        av_frame_free(&m_vInFrame);
        m_vInFrame = nullptr;
    }
    if (m_vInFrameBuf) {
        av_free(m_vInFrameBuf);
        m_vInFrameBuf = nullptr;
    }
    if (m_vOutFrame) {
        av_frame_free(&m_vOutFrame);
        m_vOutFrame = nullptr;
    }
    if (m_vOutFrameBuf) {
        av_free(m_vOutFrameBuf);
        m_vOutFrameBuf = nullptr;
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
}

int VideoFrameQueue::writeFrame(AVFrame* oldFrame, const VideoCaptureInfo& info, int64_t captureTime) {
    if (!m_isInit) return -1;
    if (!oldFrame) return -1;

    if (info.width != m_videoCapInfo.width || info.height != m_videoCapInfo.height || info.format != m_videoCapInfo.format) {
        m_videoCapInfo = info;

        if (m_videoCapInfo.width != m_width || m_videoCapInfo.height != m_height || m_videoCapInfo.format != m_format) {
            m_needScale = true;
            m_swsCtx    = sws_getContext(m_videoCapInfo.width, m_videoCapInfo.height, m_videoCapInfo.format,
                                      m_width, m_height, m_format,
                                      SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
        }
    }

    if (m_needScale && m_swsCtx) {
        // srcSliceH应该是输入高度，return输出高度
        int h = sws_scale(m_swsCtx, (const uint8_t* const*)oldFrame->data, oldFrame->linesize, 0,
                          m_videoCapInfo.height, m_vInFrame->data, m_vInFrame->linesize);
    }

    //static int s_cnt = 1;
    //QTime t = QTime::currentTime();
    {
        unique_lock<mutex> lk(m_mtxVBuf);
        m_cvVBufNotFull.wait(lk, [this] { return av_fifo_space(m_vFifoBuf) >= m_vFrameItemSize; });
    }
    //qDebug() << "m_cvVBufNotFull.wait duration:" << t.elapsed() << " time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << s_cnt++;

    av_fifo_generic_write(m_vFifoBuf, &captureTime, sizeof(int64_t), NULL);
    if (m_needScale) {
        av_fifo_generic_write(m_vFifoBuf, m_vInFrameBuf, m_vFrameSize, NULL);
    } else {
        av_fifo_generic_write(m_vFifoBuf, oldFrame->data[0], m_vFrameSize, NULL);
    }
    //qDebug() << "av_fifo_generic_write 2";
    m_cvVBufNotEmpty.notify_one();
    //qDebug() << "av_fifo_generic_write time:" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "capture time:" << captureTime;
    return 0;
}

FrameItem* VideoFrameQueue::readFrame() {
    if (!m_isInit) return nullptr;

    {
        unique_lock<mutex> lk(m_mtxVBuf);
        //m_cvVBufNotEmpty.wait(lk, [this] { return av_fifo_size(m_vFifoBuf) >= m_vFrameItemSize; });
        bool notTimeout = m_cvVBufNotEmpty.wait_for(lk, 100ms, [this] { return av_fifo_size(m_vFifoBuf) >= m_vFrameItemSize; });
        if (!notTimeout) {
            //qDebug() << "Video wait timeout";
            return nullptr;
        }
    }
    int64_t captureTime;
    av_fifo_generic_read(m_vFifoBuf, &captureTime, sizeof(int64_t), NULL);
    av_fifo_generic_read(m_vFifoBuf, m_vOutFrameBuf, m_vFrameSize, NULL);
    m_cvVBufNotFull.notify_one();

    m_vFrameItem.captureTime = captureTime;
    m_vFrameItem.frame       = m_vOutFrame;

    return &m_vFrameItem;
}

}  // namespace onlyet