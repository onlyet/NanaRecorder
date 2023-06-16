#include "AudioFrameQueue.h"
#include "FFmpegHeader.h"
#include "FFmpegHelper.h"

#include <QDebug>
#include <QDateTime>

#include <mutex>

using namespace std;

namespace onlyet {

int AudioFrameQueue::initBuf(AVCodecContext* encodeCtx) {
    if (!encodeCtx) return -1;

    m_channelLayout = encodeCtx->channel_layout;
    m_format        = encodeCtx->sample_fmt;
    m_sampleRate    = encodeCtx->sample_rate;
    m_channelNum    = av_get_channel_layout_nb_channels(m_channelLayout);

    m_aOutFrame                 = av_frame_alloc();
    m_aOutFrame->format         = encodeCtx->sample_fmt;
    m_aOutFrame->nb_samples     = encodeCtx->frame_size;
    m_aOutFrame->channel_layout = encodeCtx->channel_layout;

    int ret = av_frame_get_buffer(m_aOutFrame, 0);
    if (ret < 0) {
        qCritical() << "av_frame_get_buffer failed";
        return -1;
    }
    // 一帧16K，分配100帧则1.6M
    m_aFifoBuf = av_audio_fifo_alloc(encodeCtx->sample_fmt, encodeCtx->channels,
                                     100 * encodeCtx->frame_size);
    if (!m_aFifoBuf) {
        qCritical() << "av_audio_fifo_alloc failed";
        return -1;
    }

    memset(m_resampleBuf, 0, sizeof(m_resampleBuf));

    m_isInit = true;
    return 0;
}

void AudioFrameQueue::deinit() {
    if (m_swrCtx) {
        swr_free(&m_swrCtx);
    }
    if (m_aOutFrame) {
        av_frame_free(&m_aOutFrame);
    }
    if (m_aFifoBuf) {
        av_audio_fifo_free(m_aFifoBuf);
        m_aFifoBuf = nullptr;
    }
    if (m_resampleBuf[0]) {
        av_freep(&m_resampleBuf[0]);
        memset(m_resampleBuf, 0, sizeof(m_resampleBuf));
    }
}

int AudioFrameQueue::writeFrame(AVFrame* oldFrame, const AudioCaptureInfo& info) {
    if (!m_isInit) return -1;
    if (!oldFrame) return -1;

    int ret = -1;
    if (info.channelLayout != m_audioCapInfo.channelLayout || info.format != m_audioCapInfo.format || info.sampleRate != m_audioCapInfo.sampleRate) {
        m_audioCapInfo = info;

        m_swrCtx = swr_alloc();
        if (!m_swrCtx) return -1;
        av_opt_set_channel_layout(m_swrCtx, "in_channel_layout", m_audioCapInfo.channelLayout, 0);
        av_opt_set_channel_layout(m_swrCtx, "out_channel_layout", m_channelLayout, 0);
        av_opt_set_int(m_swrCtx, "in_sample_rate", m_audioCapInfo.sampleRate, 0);
        av_opt_set_int(m_swrCtx, "out_sample_rate", m_sampleRate, 0);
        av_opt_set_sample_fmt(m_swrCtx, "in_sample_fmt", m_audioCapInfo.format, 0);
        av_opt_set_sample_fmt(m_swrCtx, "out_sample_fmt", m_format, 0);
        swr_init(m_swrCtx);
        if ((ret = swr_init(m_swrCtx)) < 0) {
            qCritical() << "swr_init failed";
            return -1;
        }
    }

    int64_t dst_nb_samples = av_rescale_rnd(oldFrame->nb_samples + swr_get_delay(m_swrCtx, info.sampleRate),
                                            m_sampleRate, info.sampleRate, AV_ROUND_UP);
    if (dst_nb_samples <= 0) {
        qCritical() << "av_rescale_rnd failed";
        return -1;
    }

    if (dst_nb_samples > m_resampleBufSize) {
        if (m_resampleBuf[0]) {
            // 整个buf都会被释放，不需要每个通道都释放一次
            av_freep(&m_resampleBuf[0]);
        }

        ret = av_samples_alloc(m_resampleBuf, NULL, m_channelNum, dst_nb_samples, m_format, 0);
        if (ret < 0) {
            qCritical() << "av_samples_alloc failed";
            return -1;
        }
        m_resampleBufSize = dst_nb_samples;
    }

    int outSampleNum = swr_convert(m_swrCtx, m_resampleBuf, dst_nb_samples, (const uint8_t**)oldFrame->data, oldFrame->nb_samples);
    if (outSampleNum <= 0) {
        qCritical() << "swr_convert failed";
        return -1;
    }

    //static int s_cnt = 1;
    //QTime      t     = QTime::currentTime();
    {
        unique_lock<mutex> lk(m_mtxABuf);
        m_cvABufNotFull.wait(lk, [this, &outSampleNum] { return av_audio_fifo_space(m_aFifoBuf) >= outSampleNum; });
    }
    //qDebug() << "m_cvVBufNotFull.wait duration:" << t.elapsed() << " time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << s_cnt++;

    av_audio_fifo_write(m_aFifoBuf, (void**)m_resampleBuf, outSampleNum);
    m_cvABufNotEmpty.notify_one();
    //qDebug() << "av_audio_fifo_write time:" << QDateTime::currentDate Time().toString("yyyy-MM-dd hh:mm:ss.zzz");

    return 0;
}

AVFrame* AudioFrameQueue::readFrame() {
    if (!m_isInit) return nullptr;

    {
        unique_lock<mutex> lk(m_mtxABuf);
        bool               notTimeout = m_cvABufNotEmpty.wait_for(lk, 100ms, [this] { return av_audio_fifo_size(m_aFifoBuf) >= m_aOutFrame->nb_samples; });
        if (!notTimeout) {
            //qDebug() << "timeout, av_audio_fifo_size:" << av_audio_fifo_size(m_aFifoBuf);
            return nullptr;
        }

        // 从FIFO读取到n个平面（n是通道数，一个通道一个平面）
        //int tt = av_audio_fifo_size(m_aFifoBuf);
        //qDebug() << "1 av_audio_fifo_size:" << tt;
        int nread = av_audio_fifo_read(m_aFifoBuf, (void**)m_aOutFrame->data, m_aOutFrame->nb_samples);
        if (nread != m_aOutFrame->nb_samples) {
            qCritical() << "av_audio_fifo_read failed:" << FFmpegHelper::err2Str(nread);
            return nullptr;
        }
        //int tt2 = av_audio_fifo_size(m_aFifoBuf);
        //qDebug() << "2 av_audio_fifo_size:" << tt2;
    }
    m_cvABufNotFull.notify_one();

    return m_aOutFrame;
}

int AudioFrameQueue::writeFrame(AVFrame* frame) {
    if (!m_isInit) return -1;
    if (!frame) return -1;

    {
        unique_lock<mutex> lk(m_mtxABuf);
        // TODO: 这里改成frame->nb_samples是否正确？
        m_cvABufNotFull.wait(lk, [this, frame] { return av_audio_fifo_space(m_aFifoBuf) >= frame->nb_samples; });

        av_audio_fifo_write(m_aFifoBuf, (void**)frame->data, frame->nb_samples);
    }

    m_cvABufNotEmpty.notify_one();

    return 0;
}

}  // namespace onlyet