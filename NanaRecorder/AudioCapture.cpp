#include "AudioCapture.h"
#include "RecordConfig.h"

#include <QDebug>

#include <string>

using namespace std;

int AudioCapture::initCapture()
{
    int fps    = g_record.fps;
    int width  = g_record.width;
    int height = g_record.height;

    int ret               = -1;
    AVDictionary* options = nullptr;
    AVCodec* decoder      = nullptr;
    AVInputFormat* ifmt   = av_find_input_format("gdigrab");
    string resolution     = QString("%1x%2").arg(width).arg(height).toStdString();

    av_dict_set(&options, "framerate", QString::number(fps).toStdString().c_str(), NULL);
    av_dict_set(&options, "video_size", resolution.c_str(), 0);

    if (avformat_open_input(&m_vFmtCtx, "desktop", ifmt, &options) != 0) {
        char errbuf[1024] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf) - 1);
        qDebug() << "video avformat_open_input failed:" << errbuf;
        return -1;
    }
    if (avformat_find_stream_info(m_vFmtCtx, nullptr) < 0) {
        qDebug() << "Couldn't find stream information";
        return -1;
    }
    for (int i = 0; i < m_vFmtCtx->nb_streams; ++i) {
        AVStream* stream = m_vFmtCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (decoder == nullptr) {
                qDebug() << "can not find decoder";
                return -1;
            }
            //从视频流中拷贝参数到codecCtx
            m_vDecodeCtx = avcodec_alloc_context3(decoder);
            if ((ret = avcodec_parameters_to_context(m_vDecodeCtx, stream->codecpar)) < 0) {
                qDebug() << "Video avcodec_parameters_to_context failed,error code: " << ret;
                return -1;
            }
            m_vIndex = i;
            break;
        }
    }
    if (avcodec_open2(m_vDecodeCtx, decoder, nullptr) < 0) {
        qDebug() << "Could not open codec";
        return -1;
    }

    return 0;
}
