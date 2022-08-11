#include "AudioCapture.h"
#include "RecordConfig.h"
#include "FFmpegHelper.h"

#include <QDebug>

#include <string>

#include <Windows.h>

using namespace std;

int AudioCapture::startCapture() {
    if (m_isRunning) return -1;

    m_isRunning = true;
    int ret     = initCapture();
    if (0 != ret) {
        return -1;
    }
    std::thread t(std::bind(&AudioCapture::audioCaptureThreadProc, this));
    m_captureThread.swap(t);
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




int AudioCapture::initCapture() {
    int            ret     = -1;
    AVDictionary*  options = nullptr;
    AVCodec*       decoder = nullptr;
    AVInputFormat* ifmt    = av_find_input_format("dshow");
#if 0
    string         audioDeviceName = FFmpegHelper::getAudioDevice(AudioCaptureDevice_Speaker);
#else
    string audioDeviceName = FFmpegHelper::getAudioDevice(AudioCaptureDevice_Microphone);
#endif
    if ((ret = avformat_open_input(&m_aFmtCtx, audioDeviceName.c_str(), ifmt, &options)) != 0) {
        qDebug() << "Auido avformat_open_input failed:" << FFmpegHelper::err2Str(ret);
        return -1;
    }
    if (avformat_find_stream_info(m_aFmtCtx, nullptr) < 0) {
        qDebug() << "Couldn't find stream information";
        return -1;
    }
    for (int i = 0; i < m_aFmtCtx->nb_streams; ++i) {
        AVStream* stream = m_aFmtCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (decoder == nullptr) {
                qDebug() << "can not find decoder";
                return -1;
            }
            //从音频流中拷贝参数到codecCtx
            m_aDecodeCtx = avcodec_alloc_context3(decoder);
            if ((ret = avcodec_parameters_to_context(m_aDecodeCtx, stream->codecpar)) < 0) {
                qDebug() << "Audio avcodec_parameters_to_context failed,error code: " << ret;
                return -1;
            }
            m_aIndex = i;
            break;
        }
    }
    if (avcodec_open2(m_aDecodeCtx, decoder, nullptr) < 0) {
        qDebug() << "Could not open codec";
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

void AudioCapture::audioCaptureThreadProc() {
    if (!m_frameCb) {
        qDebug() << "m_frameCb empty, thread exit";
        return;
    }

    int      ret = -1;
    AVPacket pkt = {0};
    av_init_packet(&pkt);
    AVFrame* oldFrame = av_frame_alloc();

    while (m_isRunning) {
        if (g_record.status == RecordStatus::Paused) {
            unique_lock<mutex> lk(g_record.mtxPause);
            g_record.cvNotPause.wait(lk, [this] { return g_record.status != RecordStatus::Paused; });
        }

        if (!m_aFmtCtx || !m_aDecodeCtx) {
            qDebug() << "m_aFmtCtx or m_aDecodeCtx nullptr";
            break;
        }
        //static int s_cnt = 1;
        //QTime      t     = QTime::currentTime();
        if (av_read_frame(m_aFmtCtx, &pkt) < 0) {
            qDebug() << "Audio av_read_frame < 0";
            continue;
        }
        //qDebug() << "audio pkt: " << pkt.pts << "," << pkt.dts;
        //qDebug() << "av_read_frame duration:" << t.elapsed() << " time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << s_cnt++;

        if (pkt.stream_index != m_aIndex) {
            qDebug() << "not a Audio packet from Audio input";
            av_packet_unref(&pkt);
            continue;
        }
        ret = avcodec_send_packet(m_aDecodeCtx, &pkt);
        if (ret != 0) {
            qDebug() << "Audio avcodec_send_packet failed, ret:" << ret;
            av_packet_unref(&pkt);
            continue;
        }
        ret = avcodec_receive_frame(m_aDecodeCtx, oldFrame);
        if (ret != 0) {
            qDebug() << "Audio avcodec_receive_frame failed, ret:" << ret;
            av_packet_unref(&pkt);
            continue;
        }
        av_packet_unref(&pkt);

        AudioCaptureInfo info;
        // 手动设置布局，因为从流中获取的通道布局是0
        info.channelLayout = /*m_aDecodeCtx->channel_layout*/ AV_CH_LAYOUT_STEREO;
        info.format        = m_aDecodeCtx->sample_fmt;
        info.sampleRate    = m_aDecodeCtx->sample_rate;
        m_frameCb(oldFrame, info);
    }
    //FlushAudioDecoder();

    av_frame_free(&oldFrame);
    qDebug() << "audioCaptureThreadProc thread exit";
}
