#include "mux.h"
#include "FFmpegHeader.h"
#include "FFmpegHelper.h"

#include <QDebug>
#include <QTime>
#include <QDateTime>

using namespace std;

namespace onlyet {

int Mux::init(const std::string& filename) {
    m_filename              = filename;
    const char* outFileName = m_filename.c_str();
    int         ret         = avformat_alloc_output_context2(&m_oFmtCtx, nullptr, nullptr, outFileName);
    if (ret < 0) {
        qCritical() << "avformat_alloc_output_context2 failed";
        return -1;
    }
    m_isInit = true;
    return 0;
}

void Mux::deinit() {
    m_isInit = false;
    if (m_oFmtCtx) {
        avio_close(m_oFmtCtx->pb);
        avformat_free_context(m_oFmtCtx);
        m_oFmtCtx = nullptr;
    }
    m_vStream    = nullptr;  // avformat_free_context时释放
    m_aStream    = nullptr;
    m_vEncodeCtx = nullptr;
    m_aEncodeCtx = nullptr;
}

int Mux::writeHeader() {
    if (!m_isInit || !m_oFmtCtx) return -1;

    //打开输出文件
    if (!(m_oFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        const char* outFileName = m_filename.c_str();
        if (avio_open(&m_oFmtCtx->pb, outFileName, AVIO_FLAG_WRITE) < 0) {
            qCritical() << "can not open output file handle!";
            return -1;
        }
    }
    //写文件头
    if (avformat_write_header(m_oFmtCtx, nullptr) < 0) {
        qCritical() << "can not write the header of the output file!";
        return -1;
    }

    return 0;
}

int Mux::writePacket(AVPacket* packet, int64_t captureTime) {
    if (!packet || packet->size <= 0 || !packet->data) {
        qCritical() << "packet is null";
        if (packet) av_packet_free(&packet);

        return -1;
    }

    if (!m_isInit || !m_oFmtCtx) return -1;

    int stream_index = packet->stream_index;

    AVRational src_time_base;  // 编码后的包
    AVRational dst_time_base;  // mp4输出文件对应流的time_base
    if (m_vStream && m_vEncodeCtx && stream_index == m_vIndex) {
        src_time_base = m_vEncodeCtx->time_base;
        dst_time_base = m_vStream->time_base;
    } else if (m_aStream && m_aEncodeCtx && stream_index == m_aIndex) {
        src_time_base = m_aEncodeCtx->time_base;
        dst_time_base = m_aStream->time_base;
    }
#if 0
    packet->pts = av_rescale_q(packet->pts, src_time_base, dst_time_base);
    packet->dts = av_rescale_q(packet->dts, src_time_base, dst_time_base);
    packet->duration = av_rescale_q(packet->duration, src_time_base, dst_time_base);
#else
    packet->pts = av_rescale_q(captureTime, AVRational{1, /*1000*/ 1000 * 1000}, dst_time_base);
    //qDebug() << "Index:" << stream_index << " pts:" << packet->pts << " captureTime:" << captureTime;
    packet->dts = packet->pts;
#endif

    //if (packet->stream_index == 0) {
    //    qDebug() << "av_interleaved_write_frame time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    //}

    int ret;
    //QTime t = QTime::currentTime();
    {
        lock_guard<mutex> lock(m_WriteFrameMtx);
        // av_interleaved_write_frame调用后packet的各个字段变为0
#if 0
        qDebug() << QString("av_interleaved_write_frame, stream_index=%1, pts=%2, dts=%3, duration=%4, size=%5")
                        .arg(stream_index)
                        .arg(packet->pts)
                        .arg(packet->dts)
                        .arg(packet->duration)
                        .arg(packet->size);
#endif
        // 相同dts会导致av_interleaved_write_frame返回Invalid argument（-22）
        ret = av_interleaved_write_frame(m_oFmtCtx, packet);
    }
    //qDebug() << "av_interleaved_write_frame duration:" << t.elapsed() << " time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    av_packet_free(&packet);
    if (ret != 0) {
        //qCritical() << "av_interleaved_write_frame failed, ret:" << ret;
        qCritical() << QString("stream_index=%1, av_interleaved_write_frame failed: %2").arg(stream_index).arg(FFmpegHelper::err2Str(ret));
        return -1;
    }
#if 0
    static int s_writeCnt = 0;
    ++s_writeCnt;
    qDebug() << "s_writeCnt" << s_writeCnt;
#endif
    return 0;
}

int Mux::writeTrailer() {
    if (!m_isInit || !m_oFmtCtx) return -1;
    int ret = av_write_trailer(m_oFmtCtx);
    return 0;
}

int Mux::addStream(AVCodecContext* encodeCtx) {
    if (!m_isInit || !m_oFmtCtx || !encodeCtx) return -1;

    AVStream* stream = avformat_new_stream(m_oFmtCtx, nullptr);
    if (!stream) {
        qCritical() << "avformat_new_stream failed";
        return -1;
    }

    //将codecCtx中的参数传给输出流
    int ret = avcodec_parameters_from_context(stream->codecpar, encodeCtx);
    if (ret < 0) {
        qCritical() << "Output avcodec_parameters_from_context,error code:" << ret;
        return -1;
    }

    if (encodeCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
        m_vEncodeCtx = encodeCtx;
        m_vStream    = stream;
        m_vIndex     = stream->index;
        //qInfo() << "Video stream index" << m_vIndex;
    } else if (encodeCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        m_aEncodeCtx = encodeCtx;
        m_aStream    = stream;
        m_aIndex     = stream->index;
        //qInfo() << "Audio stream index" << m_aIndex;
    }

    return 0;
}

}  // namespace onlyet