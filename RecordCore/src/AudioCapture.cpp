#include "AudioCapture.h"
#include "FFmpegHeader.h"
#include "FFmpegHelper.h"
#include "RecordConfig.h"

#include <QDebug>

#include <string>
#include <thread>

#ifdef WIN32
#define AUDIO_DEVICE_FORMAT "dshow"
#elif __linux__
#define AUDIO_DEVICE_FORMAT "pulse"
#else
#error Unsupported platform
#endif

using namespace std;

namespace onlyet {

int AudioCapture::startCapture(AudioCaptureDevice dev) {
    if (m_isRunning) return -1;

    m_isRunning = true;
    int ret     = initCapture(dev);
    if (0 != ret) {
        return -1;
    }
    m_captureThread = thread(bind(&AudioCapture::audioCaptureThread, this));
    return 0;
}

int AudioCapture::stopCapture() {
    if (!m_isRunning) return -1;

    m_isRunning = false;
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
    deinit();
    return 0;
}

AVRational AudioCapture::timebase() {
    if (m_aFmtCtx && m_aIndex != -1) {
        return m_aFmtCtx->streams[m_aIndex]->time_base;
    } else {
        return {1, AV_TIME_BASE};
    }
}

int AudioCapture::sampleRate() {
    if (m_aDecodeCtx) {
        return m_aDecodeCtx->sample_rate;
    } else {
        return 0;
    }
}

AVSampleFormat AudioCapture::sampleFormat() {
    if (m_aDecodeCtx) {
        return m_aDecodeCtx->sample_fmt;
    } else {
        return AV_SAMPLE_FMT_NONE;
    }
}

int AudioCapture::channel() {
    if (m_aDecodeCtx) {
        return m_aDecodeCtx->channels;
    } else {
        return 0;
    }
}

int64_t AudioCapture::channelLayout() {
    if (m_aDecodeCtx) {
        //return av_channel_layout_default();
        return av_get_default_channel_layout(m_aDecodeCtx->channels);
    } else {
        return 0;
    }
}

int AudioCapture::initCapture(AudioCaptureDevice dev) {
    int                  ret     = -1;
    AVDictionary*        options = nullptr;
    const AVCodec*       decoder = nullptr;
    const AVInputFormat* ifmt    = av_find_input_format(AUDIO_DEVICE_FORMAT);

    string audioDeviceName = FFmpegHelper::getAudioDevice(dev);
    if ("" == audioDeviceName) {
        qCritical() << "Can not find audio device";
        return -1;
    }
    if ((ret = avformat_open_input(&m_aFmtCtx, audioDeviceName.c_str(), ifmt, &options)) != 0) {
        qCritical() << "Auido avformat_open_input failed:" << FFmpegHelper::err2Str(ret);
        return -1;
    }
    if (avformat_find_stream_info(m_aFmtCtx, nullptr) < 0) {
        qCritical() << "Couldn't find stream information";
        return -1;
    }
    for (int i = 0; i < m_aFmtCtx->nb_streams; ++i) {
        AVStream* stream = m_aFmtCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (decoder == nullptr) {
                qCritical() << "can not find decoder";
                return -1;
            }
            //从音频流中拷贝参数到codecCtx
            m_aDecodeCtx = avcodec_alloc_context3(decoder);
            if ((ret = avcodec_parameters_to_context(m_aDecodeCtx, stream->codecpar)) < 0) {
                qCritical() << "Audio avcodec_parameters_to_context failed,error code: " << ret;
                return -1;
            }
            m_aIndex = i;
            break;
        }
    }
    if (avcodec_open2(m_aDecodeCtx, decoder, nullptr) < 0) {
        qCritical() << "Could not open codec";
        return -1;
    }

    return 0;
}

void AudioCapture::deinit() {
    if (m_aFmtCtx) {
        avformat_close_input(&m_aFmtCtx);
        m_aFmtCtx = nullptr;
    }
    if (m_aDecodeCtx) {
        avcodec_free_context(&m_aDecodeCtx);
        m_aDecodeCtx = nullptr;
    }
}

void AudioCapture::audioCaptureThread() {
    if (!m_amixFilterCb) {
        qCritical() << "m_amixFilterCb empty, thread exit";
        return;
    }

    int      ret = -1;
    AVPacket pkt = {0};
    av_init_packet(&pkt);
    AVFrame* oldFrame = av_frame_alloc();

    while (m_isRunning) {
#if 0
        /// <summary>
        /// 暂停后不读音频帧会导致几秒报错：
        /// real-time buffer [virtual-audio-capturer] [audio input]
        /// too full or near too full(84 % of size : 3041280 [rtbufsize parameter]) !frame dropped !
        /// 最终录制出来的视频会在暂停位置卡住
        /// </summary>
        if (g_record.status == RecordStatus::Paused) {
            unique_lock<mutex> lk(g_record.mtxPause);
            g_record.cvNotPause.wait(lk, [this] { return g_record.status != RecordStatus::Paused; });
        }
#endif

        if (!m_aFmtCtx || !m_aDecodeCtx) {
            qCritical() << "m_aFmtCtx or m_aDecodeCtx nullptr";
            break;
        }
        if (av_read_frame(m_aFmtCtx, &pkt) < 0) {
            qCritical() << "Audio av_read_frame < 0";
            continue;
        }
        // 暂停后读packet但不处理
        if (g_record.status != RecordStatus::Running) {
            av_packet_unref(&pkt);
            this_thread::sleep_for(1ms);
            continue;
        }

        if (pkt.stream_index != m_aIndex) {
            qCritical() << "not a Audio packet from Audio input";
            av_packet_unref(&pkt);
            continue;
        }
        ret = avcodec_send_packet(m_aDecodeCtx, &pkt);
        if (ret != 0) {
            qCritical() << "Audio avcodec_send_packet failed, ret:" << ret;
            av_packet_unref(&pkt);
            continue;
        }
        ret = avcodec_receive_frame(m_aDecodeCtx, oldFrame);
        if (ret != 0) {
            qCritical() << "Audio avcodec_receive_frame failed, ret:" << ret;
            av_packet_unref(&pkt);
            continue;
        }
        //qDebug() << QString("pkt.pts:%1,frame.pts:%2").arg(pkt.pts).arg(oldFrame->pts);
        av_packet_unref(&pkt);

        m_amixFilterCb(oldFrame, m_filterCtxIndex);
    }
    //FlushAudioDecoder();

    av_frame_free(&oldFrame);
    qInfo() << "audioCaptureThread thread exit";
}

}  // namespace onlyet