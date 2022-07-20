#include "VideoCapture.h"
#include "FFmpegHeader.h"
#include "RecordConfig.h"

#include <QString>
#include <QDebug>
#include <QTime>
#include <QDateTime>

#include <string>
#include <mutex>
//#include <chrono>
//#include <functional>

using namespace std;
using namespace std::chrono;

//VideoCapture::VideoCapture(const RecordInfo& info)
//{
//}

int VideoCapture::startCapture()
{
    if (m_isRunning) return -1;

    m_isRunning = true;
    int ret = initCapture();
    if (0 != ret) {
        return -1;
    }
    std::thread t(std::bind(&VideoCapture::videoCaptureThreadProc, this));
    m_captureThread.swap(t);
    //m_captureThread.swap(std::thread(std::bind(&VideoCapture::videoCaptureThreadProc, this)));
    return 0;
}

int VideoCapture::stopCapture()
{
    if (!m_isRunning) return -1;

    m_isRunning = false;
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
    deinit();
    return 0;
}

int VideoCapture::initCapture()
{
    av_register_all();
    avdevice_register_all();
    avcodec_register_all();

    int fps = g_record.fps;
    int width = g_record.width;
    int height = g_record.height;

    int ret = -1;
    AVDictionary* options = nullptr;
    AVCodec* decoder = nullptr;
    AVInputFormat* ifmt = av_find_input_format("gdigrab");
    string resolution = QString("%1x%2").arg(width).arg(height).toStdString();

    av_dict_set(&options, "framerate", QString::number(fps).toStdString().c_str(), NULL);
    av_dict_set(&options, "video_size", resolution.c_str(), 0);

    if (avformat_open_input(&m_vFmtCtx, "desktop", ifmt, &options) != 0)
    {
        char errbuf[1024] = { 0 };
        av_strerror(ret, errbuf, sizeof(errbuf) - 1);
        qDebug() << "video avformat_open_input failed:" << errbuf;
        return -1;
    }
    if (avformat_find_stream_info(m_vFmtCtx, nullptr) < 0)
    {
        qDebug() << "Couldn't find stream information";
        return -1;
    }
    for (int i = 0; i < m_vFmtCtx->nb_streams; ++i)
    {
        AVStream* stream = m_vFmtCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (decoder == nullptr)
            {
                qDebug() << "can not find decoder";
                return -1;
            }
            //从视频流中拷贝参数到codecCtx
            m_vDecodeCtx = avcodec_alloc_context3(decoder);
            if ((ret = avcodec_parameters_to_context(m_vDecodeCtx, stream->codecpar)) < 0)
            {
                qDebug() << "Video avcodec_parameters_to_context failed,error code: " << ret;
                return -1;
            }
            m_vIndex = i;
            break;
        }
    }
    if (avcodec_open2(m_vDecodeCtx, decoder, nullptr) < 0)
    {
        qDebug() << "Could not open codec";
        return -1;
    }

    return 0;
}

void VideoCapture::deinit()
{
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
void VideoCapture::videoCaptureThreadProc()
{
    int width = g_record.width;
    int height = g_record.height;

    int ret = -1;
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    int y_size = width * height;
    AVFrame* oldFrame = av_frame_alloc();

    while (m_isRunning)
    {
        if (g_record.status == RecordStatus::Paused)
        {
            unique_lock<mutex> lk(g_record.mtxPause);
            g_record.cvNotPause.wait(lk, [this] { return g_record.status != RecordStatus::Paused; });
        }

        if (!m_vFmtCtx || !m_vDecodeCtx) {
            qDebug() << "m_vFmtCtx or m_vDecodeCtx nullptr";
            break;
        }
        static int s_cnt = 1;
        QTime t = QTime::currentTime();
        if (av_read_frame(m_vFmtCtx, &pkt) < 0)
        {
            qDebug() << "video av_read_frame < 0";
            continue;
        }
        qDebug() << "av_read_frame duration:" << t.elapsed() << " time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << s_cnt++;

        if (pkt.stream_index != m_vIndex)
        {
            qDebug() << "not a video packet from video input";
            av_packet_unref(&pkt);
        }
        ret = avcodec_send_packet(m_vDecodeCtx, &pkt);
        if (ret != 0)
        {
            qDebug() << "video avcodec_send_packet failed, ret:" << ret;
            av_packet_unref(&pkt);
            continue;
        }
        ret = avcodec_receive_frame(m_vDecodeCtx, oldFrame);
        if (ret != 0)
        {
            qDebug() << "video avcodec_receive_frame failed, ret:" << ret;
            av_packet_unref(&pkt);
            continue;
        }
        av_packet_unref(&pkt);

        VideoCaptureInfo info;
        info.width = m_vDecodeCtx->width;
        info.height = m_vDecodeCtx->height;
        info.format = m_vDecodeCtx->pix_fmt;
        m_frameCb(oldFrame, info);

        //int a = 3;
        //qDebug() << "a:" << a;
    }
    //FlushVideoDecoder();

    av_frame_free(&oldFrame);
    qDebug() << "screen record thread exit";
}
