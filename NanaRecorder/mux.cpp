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

int Mux::writePacket()
{
    return 0;
}

int Mux::writeTrailer()
{
    return 0;
}

int Mux::addStream()
{
    if (m_vFmtCtx->streams[m_vIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        AVStream* vStream = nullptr, * aStream = nullptr;
        vStream = avformat_new_stream(m_oFmtCtx, nullptr);
        if (!vStream)
        {
            printf("can not new stream for output!\n");
            return -1;
        }
        //AVFormatContext第一个创建的流索引是0，第二个创建的流索引是1
        m_vOutIndex = vStream->index;
        vStream->time_base = AVRational{ 1, m_fps };

        //将codecCtx中的参数传给输出流
        ret = avcodec_parameters_from_context(vStream->codecpar, m_vEncodeCtx);
        if (ret < 0)
        {
            qDebug() << "Output avcodec_parameters_from_context,error code:" << ret;
            return -1;
        }
    }

    return 0;
}
