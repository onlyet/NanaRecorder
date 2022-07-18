#include "VideoEncoder.h"

#include <QDebug>

int VideoEncoder::initH264(int width, int height, int fps)
{
    int ret = -1;

    m_vEncodeCtx = avcodec_alloc_context3(NULL);
    if (nullptr == m_vEncodeCtx)
    {
        qDebug() << "avcodec_alloc_context3 failed";
        return -1;
    }
    m_vEncodeCtx->width = width;
    m_vEncodeCtx->height = height;
    m_vEncodeCtx->time_base.num = 1;
    m_vEncodeCtx->time_base.den = fps;
    m_vEncodeCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    m_vEncodeCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_vEncodeCtx->codec_id = AV_CODEC_ID_H264;
    m_vEncodeCtx->bit_rate = 800 * 1000;
    m_vEncodeCtx->rc_max_rate = 800 * 1000;
    m_vEncodeCtx->rc_buffer_size = 500 * 1000;
    //����ͼ�����Ĵ�С, gop_sizeԽ���ļ�ԽС 
    m_vEncodeCtx->gop_size = 30;
    m_vEncodeCtx->max_b_frames = 0;
    //����h264����صĲ���,������avcodec_open2��ʧ��
    m_vEncodeCtx->qmin = 10;	//2
    m_vEncodeCtx->qmax = 31;	//31
    m_vEncodeCtx->max_qdiff = 4;
    m_vEncodeCtx->me_range = 16;	//0	
    m_vEncodeCtx->max_qdiff = 4;	//3	
    m_vEncodeCtx->qcompress = 0.6;	//0.5
    m_vEncodeCtx->codec_tag = 0;
    //��ȷ����sps/pps
    m_vEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //������Ƶ������
    AVCodec* encoder;
    encoder = avcodec_find_encoder(m_vEncodeCtx->codec_id);
    if (!encoder)
    {
        qDebug() << "Can not find the encoder, id: " << m_vEncodeCtx->codec_id;
        return -1;
    }
    //����Ƶ������
    ret = avcodec_open2(m_vEncodeCtx, encoder, nullptr);
    if (ret < 0)
    {
        qDebug() << "Can not open encoder id: " << encoder->id << "error code: " << ret;
        return -1;
    }
    return 0;
}

int VideoEncoder::encode()
{

    return 0;
}
