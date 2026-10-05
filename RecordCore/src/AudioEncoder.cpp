#include "FFmpegHeader.h"
#include "AudioEncoder.h"
#include "RecordConfig.h"
#include "FFmpegHelper.h"

#include <QDebug>

namespace onlyet {

int AudioEncoder::initAAC() {
    m_channel = g_record.channel;

    m_aEncodeCtx = avcodec_alloc_context3(NULL);
    if (nullptr == m_aEncodeCtx) {
        qCritical() << "avcodec_alloc_context3 failed";
        return -1;
    }
    // Why audio does not require setting the timebase
    //m_aEncodeCtx->bit_rate          = m_audioBitrate;
    m_aEncodeCtx->codec_type     = AVMEDIA_TYPE_AUDIO;
    m_aEncodeCtx->sample_fmt     = AV_SAMPLE_FMT_FLTP;
    m_aEncodeCtx->codec_id       = AV_CODEC_ID_AAC;
    m_aEncodeCtx->sample_rate    = g_record.sampleRate;
    m_aEncodeCtx->channel_layout = av_get_default_channel_layout(m_channel);
    m_aEncodeCtx->channels       = m_channel;
    // Correctly configure/set SPS and PPS (Sequence Parameter Set / Picture Parameter Set)
    m_aEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

#if 0
    av_dict_set(&m_dict, "profile", "high", 0);
    // Adjust the balance between encoding speed and quality using the --preset parameter.
    av_dict_set(&m_dict, "preset", "superfast", 0);
    av_dict_set(&m_dict, "threads", "0", 0);
    av_dict_set(&m_dict, "crf", "26", 0);
    // zerolatency: Zero latency; used in scenarios requiring extremely low latency, such as encoding for video conferencing.
    av_dict_set(&m_dict, "tune", "zerolatency", 0);
#endif

    // Find the audio encoder
    const AVCodec* encoder;
    encoder = avcodec_find_encoder(m_aEncodeCtx->codec_id);
    if (!encoder) {
        qCritical() << "Can not find the encoder, id: " << m_aEncodeCtx->codec_id;
        return -1;
    }
    // Open the audio encoder
    int ret = avcodec_open2(m_aEncodeCtx, encoder, &m_dict);
    if (ret < 0) {
        qCritical() << "Can not open encoder id: " << encoder->id << "error code: " << ret;
        return -1;
    }
    return 0;
}

void AudioEncoder::deinit() {
    if (m_aEncodeCtx) {
        avcodec_free_context(&m_aEncodeCtx);
        m_aEncodeCtx = nullptr;
    }
    av_dict_free(&m_dict);
}

int AudioEncoder::encode(AVFrame* frame, int stream_index, int64_t pts, int64_t time_base, std::vector<AVPacket*>& packets) {
    if (!m_aEncodeCtx) return -1;

    int ret = 0;

    //static int s_cnt = 1;
    //QTime      t     = QTime::currentTime();
    ret = avcodec_send_frame(m_aEncodeCtx, frame);
    //qDebug() << "avcodec_send_frame duration:" << t.elapsed() << " time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << s_cnt++;
    if (ret != 0) {
        qCritical() << "video avcodec_send_frame failed:" << FFmpegHelper::err2Str(ret);
        return -1;
    }

    while (1) {
        AVPacket* packet     = av_packet_alloc();
        ret                  = avcodec_receive_packet(m_aEncodeCtx, packet);
        packet->stream_index = stream_index;
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            ret = 0;
            av_packet_free(&packet);
            break;
        } else if (ret < 0) {
            qCritical() << "avcodec_receive_packet failed:" << FFmpegHelper::err2Str(ret);
            av_packet_free(&packet);
            ret = -1;
            break;
        }
        //printf("h264 pts:%lld\n", packet->pts);
        packets.push_back(packet);
    }

    return ret;
}

}  // namespace onlyet
