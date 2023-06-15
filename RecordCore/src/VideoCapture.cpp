#include "VideoCapture.h"
#include "FFmpegHeader.h"
#include "FFmpegHelper.h"
#include "RecordConfig.h"

#include <QString>
#include <QDebug>
#include <QTime>
#include <QDateTime>

#include <string>
#include <mutex>

#ifdef WIN32
//#define USE_DSHOW

#ifdef USE_DSHOW
#define VIDEO_DEVICE_FORMAT "dshow"
#define VIDEO_DEVICE_NAME "video=screen-capture-recorder"
#else
#define VIDEO_DEVICE_FORMAT "gdigrab"
#define VIDEO_DEVICE_NAME "desktop"
#endif  // USE_DSHOW

#elif __linux__
#define VIDEO_DEVICE_FORMAT "x11grab"
#define VIDEO_DEVICE_NAME ":1"
#else
#error Unsupported platform
#endif

using namespace std;
using namespace std::chrono;

namespace onlyet {

int VideoCapture::startCapture() {
    if (m_isRunning) return -1;

    m_isRunning = true;
    int ret     = initCapture();
    if (0 != ret) {
        return -1;
    }
    m_captureThread = thread(bind(&VideoCapture::videoCaptureThreadProc, this));
    return 0;
}

int VideoCapture::stopCapture() {
    if (!m_isRunning) return -1;

    m_isRunning = false;
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
    deinit();
    return 0;
}

int VideoCapture::initCapture() {
    int fps      = g_record.fps;
    int inWidth  = g_record.inWidth;
    int inHeight = g_record.inHeight;

    int                  ret     = -1;
    AVDictionary*        options = nullptr;
    const AVCodec*       decoder = nullptr;
    const AVInputFormat* ifmt    = av_find_input_format(VIDEO_DEVICE_FORMAT);

    av_dict_set(&options, "framerate", QString::number(fps).toStdString().c_str(), 0);
    av_dict_set(&options, "video_size", QString("%1x%2").arg(inWidth).arg(inHeight).toStdString().c_str(), 0);
#ifdef USE_DSHOW
    av_dict_set(&options, "pixel_format", "yuv420p", 0);
#endif

    if ((ret = avformat_open_input(&m_vFmtCtx, VIDEO_DEVICE_NAME, ifmt, &options)) < 0) {
        qCritical() << "video avformat_open_input failed:" << FFmpegHelper::err2Str(ret);
        return -1;
    }
    if (avformat_find_stream_info(m_vFmtCtx, nullptr) < 0) {
        qCritical() << "Couldn't find stream information";
        return -1;
    }
    for (int i = 0; i < m_vFmtCtx->nb_streams; ++i) {
        AVStream* stream = m_vFmtCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (decoder == nullptr) {
                qCritical() << "can not find decoder";
                return -1;
            }
            //从视频流中拷贝参数到codecCtx
            m_vDecodeCtx = avcodec_alloc_context3(decoder);
            if ((ret = avcodec_parameters_to_context(m_vDecodeCtx, stream->codecpar)) < 0) {
                qCritical() << "Video avcodec_parameters_to_context failed,error code: " << ret;
                return -1;
            }
            m_vIndex = i;
            break;
        }
    }
    if (avcodec_open2(m_vDecodeCtx, decoder, nullptr) < 0) {
        qCritical() << "Could not open codec";
        return -1;
    }

    return 0;
}

void VideoCapture::deinit() {
    if (m_vFmtCtx) {
        avformat_close_input(&m_vFmtCtx);
        m_vFmtCtx = nullptr;
    }
    if (m_vDecodeCtx) {
        avcodec_free_context(&m_vDecodeCtx);
        m_vDecodeCtx = nullptr;
    }
}

// TODO: flush
void VideoCapture::videoCaptureThreadProc() {
    if (!m_frameCb) {
        qCritical() << "m_frameCb empty, thread exit";
        return;
    }

    int      ret = -1;
    AVPacket pkt = {0};
    av_init_packet(&pkt);
    AVFrame* oldFrame = av_frame_alloc();

    VideoCaptureInfo info;
    info.width  = m_vDecodeCtx->width;
    info.height = m_vDecodeCtx->height;
    info.format = m_vDecodeCtx->pix_fmt;

    while (m_isRunning) {
        if (g_record.status == RecordStatus::Paused) {
            unique_lock<mutex> lk(g_record.mtxPause);
            g_record.cvNotPause.wait(lk, [this] { return g_record.status != RecordStatus::Paused; });
        }

        if (!m_vFmtCtx || !m_vDecodeCtx) {
            qCritical() << "m_vFmtCtx or m_vDecodeCtx nullptr";
            break;
        }
        //static int s_cnt = 1;
        //QTime t = QTime::currentTime();
        if (av_read_frame(m_vFmtCtx, &pkt) < 0) {
            qCritical() << "video av_read_frame < 0";
            continue;
        }
        //qDebug() << "video pkt: " << pkt.pts << "," << pkt.dts;
        //qDebug() << "av_read_frame duration:" << t.elapsed() << " time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << s_cnt++;

        if (pkt.stream_index != m_vIndex) {
            qCritical() << "not a video packet from video input";
            av_packet_unref(&pkt);
            continue;
        }
        ret = avcodec_send_packet(m_vDecodeCtx, &pkt);
        if (ret != 0) {
            qCritical() << "video avcodec_send_packet failed, ret:" << ret;
            av_packet_unref(&pkt);
            continue;
        }
        ret = avcodec_receive_frame(m_vDecodeCtx, oldFrame);
        if (ret != 0) {
            qCritical() << "video avcodec_receive_frame failed, ret:" << ret;
            av_packet_unref(&pkt);
            continue;
        }
        av_packet_unref(&pkt);

        m_frameCb(oldFrame, info);
    }
    //FlushVideoDecoder();

    av_frame_free(&oldFrame);
    qInfo() << "videoCaptureThreadProc thread exit";
}

}  // namespace onlyet