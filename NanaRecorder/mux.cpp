#include "mux.h"

#include <QDebug>

using namespace std;

int Mux::init(const std::string& filename)
{
    m_filename = filename;
    const char* outFileName = m_filename.c_str();
    int ret = avformat_alloc_output_context2(&m_oFmtCtx, nullptr, nullptr, outFileName);
    if (ret < 0)
    {
        qDebug() << "avformat_alloc_output_context2 failed";
        return -1;
    }

    return 0;
}

void Mux::deinit()
{
}

int Mux::writeHeader()
{
    //打开输出文件
    if (!(m_oFmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        const char* outFileName = m_filename.c_str();
        if (avio_open(&m_oFmtCtx->pb, outFileName, AVIO_FLAG_WRITE) < 0)
        {
            printf("can not open output file handle!\n");
            return -1;
        }
    }
    //写文件头
    if (avformat_write_header(m_oFmtCtx, nullptr) < 0)
    {
        printf("can not write the header of the output file!\n");
        return -1;
    }

    return 0;
}

int Mux::writePacket(AVPacket* packet)
{
    int stream_index = packet->stream_index;
    //printf("index:%d, pts:%lld\n", stream_index, packet->pts);

    if (!packet || packet->size <= 0 || !packet->data) {
        qDebug() << "packet is null";
        if (packet) av_packet_free(&packet);

        return -1;
    }

    AVRational src_time_base;   // 编码后的包
    AVRational dst_time_base;   // mp4输出文件对应流的time_base
    if (m_vStream && m_vEncodeCtx && stream_index == m_vIndex) {
        src_time_base = m_vEncodeCtx->time_base;
        dst_time_base = m_vStream->time_base;
    }
    else if (m_aStream && m_aEncodeCtx && stream_index == m_aIndex) {
        src_time_base = m_aEncodeCtx->time_base;
        dst_time_base = m_aStream->time_base;
    }
    // 时间基转换
    packet->pts = av_rescale_q(packet->pts, src_time_base, dst_time_base);
    packet->dts = av_rescale_q(packet->dts, src_time_base, dst_time_base);
    packet->duration = av_rescale_q(packet->duration, src_time_base, dst_time_base);

    int ret = av_interleaved_write_frame(m_oFmtCtx, packet);
    //av_free_packet(&packet);
    av_packet_free(&packet);
    if (ret != 0) {
		qDebug() << "video av_interleaved_write_frame failed, ret:" << ret;
        return -1;
    }
    return 0;
}

int Mux::writeTrailer()
{
    int ret = av_write_trailer(m_oFmtCtx);
    return 0;
}

int Mux::addStream(AVCodecContext* encodeCtx)
{
    AVStream* stream = avformat_new_stream(m_oFmtCtx, nullptr);
	if (!stream)
	{
        qDebug() << "avformat_new_stream failed";
		return -1;
	}
	//AVFormatContext第一个创建的流索引是0，第二个创建的流索引是1
	//m_vOutIndex = vStream->index;
	//vStream->time_base = AVRational{ 1, m_fps };

	//将codecCtx中的参数传给输出流
	int ret = avcodec_parameters_from_context(stream->codecpar, encodeCtx);
	if (ret < 0)
	{
		qDebug() << "Output avcodec_parameters_from_context,error code:" << ret;
		return -1;
	}


    if (encodeCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
        m_vEncodeCtx = encodeCtx;
        m_vStream = stream;
        m_vIndex = stream->index;
    }
    else if (encodeCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        m_aEncodeCtx = encodeCtx;
        m_aStream = stream;
        m_aIndex = stream->index;
    }

    return 0;
}
