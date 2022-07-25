#include "AudioFrameQueue.h"

#include <QDebug>

int AudioFrameQueue::initBuf(AVCodecContext* encodeCtx) {
    if (!encodeCtx) return -1;

    m_aOutFrame = av_frame_alloc();
    m_aOutFrame->format     = encodeCtx->sample_fmt;
    m_aOutFrame->nb_samples = encodeCtx->frame_size;
    m_aOutFrame->channel_layout = encodeCtx->channel_layout;

    int ret = av_frame_get_buffer(m_aOutFrame, 0);
    if (ret < 0) {
        qDebug() << "av_frame_get_buffer failed";
        return -1;
    }
    // Ò»Ö¡16K£¬·ÖÅä100Ö¡Ôò1.6M
    m_aFifoBuf = av_audio_fifo_alloc(encodeCtx->sample_fmt, encodeCtx->channels, 
        100 * encodeCtx->frame_size);
    if (!m_aFifoBuf) {
        qDebug() << "av_audio_fifo_alloc failed";
        return -1;
    }

    m_isInit = true;
    return 0;
}

void AudioFrameQueue::deinit() {
}

int AudioFrameQueue::writeFrame(AVFrame* frame) {
    return 0;
}

AVFrame* AudioFrameQueue::readFrame() {
    return nullptr;
}
