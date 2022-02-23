#ifdef	__cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include <libavutil\avassert.h>
#ifdef __cplusplus
};
#endif

#include "ScreenRecordImpl.h"
#include <QDebug>
//#include <QAudioDeviceInfo>
#include <thread>
#include <fstream>

#include <dshow.h>

using namespace std;

int g_vCollectFrameCnt = 0;	//视频采集帧数
int g_vEncodeFrameCnt = 0;	//视频编码帧数
int g_aCollectFrameCnt = 0;	//音频采集帧数
int g_aEncodeFrameCnt = 0;	//音频编码帧数

ScreenRecordImpl::ScreenRecordImpl(QObject * parent) :
    QObject(parent)
    , m_fps(30)
    , m_vIndex(-1), m_aIndex(-1)
    , m_vFmtCtx(nullptr), m_aFmtCtx(nullptr), m_oFmtCtx(nullptr)
    , m_vDecodeCtx(nullptr), m_aDecodeCtx(nullptr)
    , m_vEncodeCtx(nullptr), m_aEncodeCtx(nullptr)
    , m_vFifoBuf(nullptr), m_aFifoBuf(nullptr)
    , m_swsCtx(nullptr)
    , m_swrCtx(nullptr)
    , m_state(RecordState::NotStarted)
    , m_vCurPts(0), m_aCurPts(0)
{
}

void ScreenRecordImpl::Init(const QVariantMap& map)
{
    m_filePath = map["filePath"].toString();
    m_width = map["width"].toInt();
    m_height = map["height"].toInt();
    m_fps = map["fps"].toInt();
    m_audioBitrate = map["audioBitrate"].toInt();
}

void ScreenRecordImpl::Start()
{
    if (m_state == RecordState::NotStarted)
    {
        qDebug() << "start record";
        m_state = RecordState::Started;
        std::thread muxThread(&ScreenRecordImpl::MuxThreadProc, this);
        muxThread.detach();
    }
    else if (m_state == RecordState::Paused)
    {
        qDebug() << "continue record";
        m_state = RecordState::Started;
        m_cvNotPause.notify_all();
    }
}

void ScreenRecordImpl::Pause()
{
    qDebug() << "pause record";
    m_state = RecordState::Paused;
}

void ScreenRecordImpl::Stop()
{
    qDebug() << "stop record";
    RecordState state = m_state;
    m_state = RecordState::Stopped;
    if (state == RecordState::Paused)
        m_cvNotPause.notify_all();
}

int ScreenRecordImpl::OpenVideo()
{
    int ret = -1;
    AVInputFormat *ifmt = av_find_input_format("gdigrab");
    AVDictionary *options = nullptr;
    AVCodec *decoder = nullptr;
    av_dict_set(&options, "framerate", QString::number(m_fps).toStdString().c_str(), NULL);

    if (avformat_open_input(&m_vFmtCtx, "desktop", ifmt, &options) != 0)
    {
        qDebug() << "Cant not open video input stream";
        return -1;
    }
    if (avformat_find_stream_info(m_vFmtCtx, nullptr) < 0)
    {
        printf("Couldn't find stream information.（无法获取视频流信息）\n");
        return -1;
    }
    for (int i = 0; i < m_vFmtCtx->nb_streams; ++i)
    {
        AVStream *stream = m_vFmtCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (decoder == nullptr)
            {
                qDebug() << QStringLiteral("没有找到视频解码器");
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
        printf("Could not open codec.（无法打开解码器）\n");
        return -1;
    }

    m_swsCtx = sws_getContext(m_vDecodeCtx->width, m_vDecodeCtx->height, m_vDecodeCtx->pix_fmt,
        m_width, m_height, AV_PIX_FMT_YUV420P,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    return 0;
}

static char *dup_wchar_to_utf8(wchar_t *w)
{
    char *s = NULL;
    int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
    s = (char *)av_malloc(l);
    if (s)
        WideCharToMultiByte(CP_UTF8, 0, w, -1, s, l, 0, 0);
    return s;
}

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

int ScreenRecordImpl::OpenAudio()
{
    int ret = -1;
    AVCodec *decoder = nullptr;
    qDebug() << GetMicrophoneDeviceName();

    AVInputFormat *ifmt = av_find_input_format("dshow");
    QString audioDeviceName = "audio=" + GetMicrophoneDeviceName();

    if (avformat_open_input(&m_aFmtCtx, audioDeviceName.toStdString().c_str(), ifmt, nullptr) < 0)
    {
        qDebug() << "Can not open audio input stream";
        return -1;
    }
    if (avformat_find_stream_info(m_aFmtCtx, nullptr) < 0)
        return -1;

    for (int i = 0; i < m_aFmtCtx->nb_streams; ++i)
    {
        AVStream * stream = m_aFmtCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (decoder == nullptr)
            {
                printf("Codec not found.（没有找到解码器）\n");
                return -1;
            }
            //从音频流中拷贝参数到codecCtx
            m_aDecodeCtx = avcodec_alloc_context3(decoder);
            if ((ret = avcodec_parameters_to_context(m_aDecodeCtx, stream->codecpar)) < 0)
            {
                qDebug() << "Audio avcodec_parameters_to_context failed,error code: " << ret;
                return -1;
            }
            m_aIndex = i;
            break;
        }
    }
    if (0 > avcodec_open2(m_aDecodeCtx, decoder, NULL))
    {
        printf("can not find or open audio decoder!\n");
        return -1;
    }
    return 0;
}

int ScreenRecordImpl::OpenOutput()
{
    int ret = -1;
    AVStream *vStream = nullptr, *aStream = nullptr;
    string fileName = m_filePath.toStdString();
    const char *outFileName = fileName.c_str();
    ret = avformat_alloc_output_context2(&m_oFmtCtx, nullptr, nullptr, outFileName);
    if (ret < 0)
    {
        qDebug() << "avformat_alloc_output_context2 failed";
        return -1;
    }

    if (m_vFmtCtx->streams[m_vIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        vStream = avformat_new_stream(m_oFmtCtx, nullptr);
        if (!vStream)
        {
            printf("can not new stream for output!\n");
            return -1;
        }
        //AVFormatContext第一个创建的流索引是0，第二个创建的流索引是1
        m_vOutIndex = vStream->index;
        vStream->time_base = AVRational{ 1, m_fps };

        m_vEncodeCtx = avcodec_alloc_context3(NULL);
        if (nullptr == m_vEncodeCtx)
        {
            qDebug() << "avcodec_alloc_context3 failed";
            return -1;
        }
        m_vEncodeCtx->width = m_width;
        m_vEncodeCtx->height = m_height;
        m_vEncodeCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        m_vEncodeCtx->time_base.num = 1;
        m_vEncodeCtx->time_base.den = m_fps;
        m_vEncodeCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        m_vEncodeCtx->codec_id = AV_CODEC_ID_H264;
        m_vEncodeCtx->bit_rate = 800 * 1000;
        m_vEncodeCtx->rc_max_rate = 800 * 1000;
        m_vEncodeCtx->rc_buffer_size = 500 * 1000;
        //设置图像组层的大小, gop_size越大，文件越小 
        m_vEncodeCtx->gop_size = 30;
        m_vEncodeCtx->max_b_frames = 3;
        //设置h264中相关的参数,不设置avcodec_open2会失败
        m_vEncodeCtx->qmin = 10;	//2
        m_vEncodeCtx->qmax = 31;	//31
        m_vEncodeCtx->max_qdiff = 4;
        m_vEncodeCtx->me_range = 16;	//0	
        m_vEncodeCtx->max_qdiff = 4;	//3	
        m_vEncodeCtx->qcompress = 0.6;	//0.5

        //查找视频编码器
        AVCodec *encoder;
        encoder = avcodec_find_encoder(m_vEncodeCtx->codec_id);
        if (!encoder)
        {
            qDebug() << "Can not find the encoder, id: " << m_vEncodeCtx->codec_id;
            return -1;
        }
        m_vEncodeCtx->codec_tag = 0;
        //正确设置sps/pps
        m_vEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        //打开视频编码器
        ret = avcodec_open2(m_vEncodeCtx, encoder, nullptr);
        if (ret < 0)
        {
            qDebug() << "Can not open encoder id: " << encoder->id << "error code: " << ret;
            return -1;
        }
        //将codecCtx中的参数传给输出流
        ret = avcodec_parameters_from_context(vStream->codecpar, m_vEncodeCtx);
        if (ret < 0)
        {
            qDebug() << "Output avcodec_parameters_from_context,error code:" << ret;
            return -1;
        }
    }
    if (m_aFmtCtx->streams[m_aIndex]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
        aStream = avformat_new_stream(m_oFmtCtx, NULL);
        if (!aStream)
        {
            printf("can not new audio stream for output!\n");
            return -1;
        }
        m_aOutIndex = aStream->index;

        AVCodec *encoder = avcodec_find_encoder(m_oFmtCtx->oformat->audio_codec);
        if (!encoder)
        {
            qDebug() << "Can not find audio encoder, id: " << m_oFmtCtx->oformat->audio_codec;
            return -1;
        }
        m_aEncodeCtx = avcodec_alloc_context3(encoder);
        if (nullptr == m_vEncodeCtx)
        {
            qDebug() << "audio avcodec_alloc_context3 failed";
            return -1;
        }
        m_aEncodeCtx->sample_fmt = encoder->sample_fmts ? encoder->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        m_aEncodeCtx->bit_rate = m_audioBitrate;
        m_aEncodeCtx->sample_rate = 44100;
        if (encoder->supported_samplerates)
        {
            m_aEncodeCtx->sample_rate = encoder->supported_samplerates[0];
            for (int i = 0; encoder->supported_samplerates[i]; ++i)
            {
                if (encoder->supported_samplerates[i] == 44100)
                    m_aEncodeCtx->sample_rate = 44100;
            }
        }
        m_aEncodeCtx->channels = av_get_channel_layout_nb_channels(m_aEncodeCtx->channel_layout);
        m_aEncodeCtx->channel_layout = AV_CH_LAYOUT_STEREO;
        if (encoder->channel_layouts)
        {
            m_aEncodeCtx->channel_layout = encoder->channel_layouts[0];
            for (int i = 0; encoder->channel_layouts[i]; ++i)
            {
                if (encoder->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    m_aEncodeCtx->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        m_aEncodeCtx->channels = av_get_channel_layout_nb_channels(m_aEncodeCtx->channel_layout);
        aStream->time_base = AVRational{ 1, m_aEncodeCtx->sample_rate };

        m_aEncodeCtx->codec_tag = 0;
        m_aEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        if (!check_sample_fmt(encoder, m_aEncodeCtx->sample_fmt))
        {
            qDebug() << "Encoder does not support sample format " << av_get_sample_fmt_name(m_aEncodeCtx->sample_fmt);
            return -1;
        }

        //打开音频编码器，打开后frame_size被设置
        ret = avcodec_open2(m_aEncodeCtx, encoder, 0);
        if (ret < 0)
        {
            qDebug() << "Can not open the audio encoder, id: " << encoder->id << "error code: " << ret;
            return -1;
        }
        //将codecCtx中的参数传给音频输出流
        ret = avcodec_parameters_from_context(aStream->codecpar, m_aEncodeCtx);
        if (ret < 0)
        {
            qDebug() << "Output audio avcodec_parameters_from_context,error code:" << ret;
            return -1;
        }

        m_swrCtx = swr_alloc();
        if (!m_swrCtx)
        {
            qDebug() << "swr_alloc failed";
            return -1;
        }
        av_opt_set_int(m_swrCtx, "in_channel_count", m_aDecodeCtx->channels, 0);	//2
        av_opt_set_int(m_swrCtx, "in_sample_rate", m_aDecodeCtx->sample_rate, 0);	//44100
        av_opt_set_sample_fmt(m_swrCtx, "in_sample_fmt", m_aDecodeCtx->sample_fmt, 0);	//AV_SAMPLE_FMT_S16
        av_opt_set_int(m_swrCtx, "out_channel_count", m_aEncodeCtx->channels, 0);	//2
        av_opt_set_int(m_swrCtx, "out_sample_rate", m_aEncodeCtx->sample_rate, 0);	//44100
        av_opt_set_sample_fmt(m_swrCtx, "out_sample_fmt", m_aEncodeCtx->sample_fmt, 0);	//AV_SAMPLE_FMT_FLTP

        if ((ret = swr_init(m_swrCtx)) < 0)
        {
            qDebug() << "swr_init failed";
            return -1;
        }
    }

    //打开输出文件
    if (!(m_oFmtCtx->oformat->flags & AVFMT_NOFILE))
    {
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

QString ScreenRecordImpl::GetSpeakerDeviceName()
{
    char sName[256] = { 0 };
    QString speaker = "";
    bool bRet = false;
    ::CoInitialize(NULL);

    ICreateDevEnum* pCreateDevEnum;//enumrate all speaker devices
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum,
        (void**)&pCreateDevEnum);

    IEnumMoniker* pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioRendererCategory, &pEm, 0);
    if (hr != NOERROR)
    {
        ::CoUninitialize();
        return "";
    }

    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;
    while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
    {

        IPropertyBag* pBag = NULL;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->Read(L"FriendlyName", &var, NULL);//还有其他属性，像描述信息等等
            if (hr == NOERROR)
            {
                //获取设备名称
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sName, 256, "", NULL);
                speaker = QString::fromLocal8Bit(sName);
                SysFreeString(var.bstrVal);
            }
            pBag->Release();
        }
        pM->Release();
        bRet = true;
    }
    pCreateDevEnum = NULL;
    pEm = NULL;
    ::CoUninitialize();
    return speaker;
}

QString ScreenRecordImpl::GetMicrophoneDeviceName()
{
    char sName[256] = { 0 };
    QString capture = "";
    bool bRet = false;
    ::CoInitialize(NULL);

    ICreateDevEnum* pCreateDevEnum;//enumrate all audio capture devices
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum,
        (void**)&pCreateDevEnum);

    IEnumMoniker* pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEm, 0);
    if (hr != NOERROR)
    {
        ::CoUninitialize();
        return "";
    }

    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;
    while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
    {

        IPropertyBag* pBag = NULL;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->Read(L"FriendlyName", &var, NULL);//还有其他属性，像描述信息等等
            if (hr == NOERROR)
            {
                //获取设备名称
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sName, 256, "", NULL);
                capture = QString::fromLocal8Bit(sName);
                SysFreeString(var.bstrVal);
            }
            pBag->Release();
        }
        pM->Release();
        bRet = true;
    }
    pCreateDevEnum = NULL;
    pEm = NULL;
    ::CoUninitialize();
    return capture;
}

AVFrame* ScreenRecordImpl::AllocAudioFrame(AVCodecContext* c, int nbSamples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    frame->format = c->sample_fmt;
    frame->channel_layout = c->channel_layout ? c->channel_layout : AV_CH_LAYOUT_STEREO;
    frame->sample_rate = c->sample_rate;
    frame->nb_samples = nbSamples;

    if (nbSamples)
    {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0)
        {
            qDebug() << "av_frame_get_buffer failed";
            return nullptr;
        }
    }
    return frame;
}

void ScreenRecordImpl::InitVideoBuffer()
{
    m_vOutFrameSize = av_image_get_buffer_size(m_vEncodeCtx->pix_fmt, m_width, m_height, 1);
    m_vOutFrameBuf = (uint8_t *)av_malloc(m_vOutFrameSize);
    m_vOutFrame = av_frame_alloc();
    //先让AVFrame指针指向buf，后面再写入数据到buf
    av_image_fill_arrays(m_vOutFrame->data, m_vOutFrame->linesize, m_vOutFrameBuf, m_vEncodeCtx->pix_fmt, m_width, m_height, 1);
    //申请30帧缓存
    if (!(m_vFifoBuf = av_fifo_alloc_array(30, m_vOutFrameSize)))
    {
        qDebug() << "av_fifo_alloc_array failed";
        return;
    }
}

void ScreenRecordImpl::InitAudioBuffer()
{
    m_nbSamples = m_aEncodeCtx->frame_size;
    if (!m_nbSamples)
    {
        qDebug() << "m_nbSamples==0";
        m_nbSamples = 1024;
    }
    m_aFifoBuf = av_audio_fifo_alloc(m_aEncodeCtx->sample_fmt, m_aEncodeCtx->channels, 30 * m_nbSamples);
    if (!m_aFifoBuf)
    {
        qDebug() << "av_audio_fifo_alloc failed";
        return;
    }
}

void ScreenRecordImpl::FlushVideoDecoder()
{
    int ret = -1;
    int y_size = m_width * m_height;
    AVFrame	*oldFrame = av_frame_alloc();
    AVFrame *newFrame = av_frame_alloc();

    ret = avcodec_send_packet(m_vDecodeCtx, nullptr);
    if (ret != 0)
    {
        qDebug() << "flush video avcodec_send_packet failed, ret: " << ret;
        return;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(m_vDecodeCtx, oldFrame);
        if (ret < 0)
        {
            if (ret == AVERROR_EOF)
            {
                qDebug() << "flush video decoder finished";
                break;
            }
            else if (ret == AVERROR(EAGAIN))
            {
                //排水模式不会EAGAIN
                qDebug() << "flush EAGAIN avcodec_receive_frame";
            }
            qDebug() << "flush video avcodec_receive_frame error, ret: " << ret;
            return;
        }
        ++g_vCollectFrameCnt;
        sws_scale(m_swsCtx, (const uint8_t* const*)oldFrame->data, oldFrame->linesize, 0,
            m_vEncodeCtx->height, newFrame->data, newFrame->linesize);

        {
            unique_lock<mutex> lk(m_mtxVBuf);
            m_cvVBufNotFull.wait(lk, [this] { return av_fifo_space(m_vFifoBuf) >= m_vOutFrameSize; });
        }
        av_fifo_generic_write(m_vFifoBuf, newFrame->data[0], y_size, NULL);
        av_fifo_generic_write(m_vFifoBuf, newFrame->data[1], y_size / 4, NULL);
        av_fifo_generic_write(m_vFifoBuf, newFrame->data[2], y_size / 4, NULL);
        m_cvVBufNotEmpty.notify_one();
    }
    av_frame_free(&oldFrame);
    av_frame_free(&newFrame);
    qDebug() << "video collect frame count: " << g_vCollectFrameCnt;
}

//void ScreenRecordImpl::FlushVideoEncoder()
//{
//	int ret = -1;
//	AVPacket pkt = { 0 };
//	av_init_packet(&pkt);
//	ret = avcodec_send_frame(m_vEncodeCtx, nullptr);
//	qDebug() << "avcodec_send_frame ret:" << ret;
//	while (ret >= 0)
//	{
//		ret = avcodec_receive_packet(m_vEncodeCtx, &pkt);
//		if (ret < 0)
//		{
//			av_packet_unref(&pkt);
//			if (ret == AVERROR(EAGAIN))
//			{
//				qDebug() << "flush EAGAIN avcodec_receive_packet";
//				ret = 1;
//				continue;
//			}
//			else if (ret == AVERROR_EOF)
//			{
//				qDebug() << "flush video encoder finished";
//				break;
//			}
//			qDebug() << "flush video avcodec_receive_packet failed, ret: " << ret;
//			return;
//		}
//		pkt.stream_index = m_vOutIndex;
//		av_packet_rescale_ts(&pkt, m_vEncodeCtx->time_base, m_oFmtCtx->streams[m_vOutIndex]->time_base);
//
//		ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
//		if (ret == 0)
//			qDebug() << "flush Write video packet id: " << ++g_vEncodeFrameCnt;
//		else
//			qDebug() << "video av_interleaved_write_frame failed, ret:" << ret;
//		av_free_packet(&pkt);
//	}
//}

void ScreenRecordImpl::FlushAudioDecoder()
{
    int ret = -1;
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    int dstNbSamples, maxDstNbSamples;
    AVFrame *rawFrame = av_frame_alloc();
    AVFrame *newFrame = AllocAudioFrame(m_aEncodeCtx, m_nbSamples);
    maxDstNbSamples = dstNbSamples = av_rescale_rnd(m_nbSamples,
        m_aEncodeCtx->sample_rate, m_aDecodeCtx->sample_rate, AV_ROUND_UP);

    ret = avcodec_send_packet(m_aDecodeCtx, nullptr);
    if (ret != 0)
    {
        qDebug() << "flush audio avcodec_send_packet  failed, ret: " << ret;
        return;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(m_aDecodeCtx, rawFrame);
        if (ret < 0)
        {
            if (ret == AVERROR_EOF)
            {
                qDebug() << "flush audio decoder finished";
                break;
            }
            else if (ret == AVERROR(EAGAIN))
            {
                //排水模式不会EAGAIN
                qDebug() << "flush audio EAGAIN avcodec_receive_frame";
            }
            qDebug() << "flush audio avcodec_receive_frame error, ret: " << ret;
            return;
        }
        ++g_aCollectFrameCnt;

        dstNbSamples = av_rescale_rnd(swr_get_delay(m_swrCtx, m_aDecodeCtx->sample_rate) + rawFrame->nb_samples,
            m_aEncodeCtx->sample_rate, m_aDecodeCtx->sample_rate, AV_ROUND_UP);
        if (dstNbSamples > maxDstNbSamples)
        {
            qDebug() << "flush audio newFrame realloc";
            av_freep(&newFrame->data[0]);
            ret = av_samples_alloc(newFrame->data, newFrame->linesize, m_aEncodeCtx->channels,
                dstNbSamples, m_aEncodeCtx->sample_fmt, 1);
            if (ret < 0)
            {
                qDebug() << "flush av_samples_alloc failed";
                return;
            }
            maxDstNbSamples = dstNbSamples;
            m_aEncodeCtx->frame_size = dstNbSamples;
            m_nbSamples = newFrame->nb_samples;
        }
        newFrame->nb_samples = swr_convert(m_swrCtx, newFrame->data, dstNbSamples,
            (const uint8_t **)rawFrame->data, rawFrame->nb_samples);
        if (newFrame->nb_samples < 0)
        {
            qDebug() << "flush swr_convert failed";
            return;
        }

        {
            unique_lock<mutex> lk(m_mtxABuf);
            m_cvABufNotFull.wait(lk, [newFrame, this] { return av_audio_fifo_space(m_aFifoBuf) >= newFrame->nb_samples; });
        }
        if (av_audio_fifo_write(m_aFifoBuf, (void **)newFrame->data, newFrame->nb_samples) < newFrame->nb_samples)
        {
            qDebug() << "av_audio_fifo_write";
            return;
        }
        m_cvABufNotEmpty.notify_one();
    }
    av_frame_free(&rawFrame);
    av_frame_free(&newFrame);
    qDebug() << "audio collect frame count: " << g_aCollectFrameCnt;
}

//void ScreenRecordImpl::FlushAudioEncoder()
//{
//}

void ScreenRecordImpl::FlushEncoders()
{
    int ret = -1;
    bool vBeginFlush = false;
    bool aBeginFlush = false;

    m_vCurPts = m_aCurPts = 0;

    int nFlush = 2;

    while (1)
    {
        AVPacket pkt = { 0 };
        av_init_packet(&pkt);
        if (av_compare_ts(m_vCurPts, m_oFmtCtx->streams[m_vOutIndex]->time_base,
            m_aCurPts, m_oFmtCtx->streams[m_aOutIndex]->time_base) <= 0)
        {
            if (!vBeginFlush)
            {
                vBeginFlush = true;
                ret = avcodec_send_frame(m_vEncodeCtx, nullptr);
                if (ret != 0)
                {
                    qDebug() << "flush video avcodec_send_frame failed, ret: " << ret;
                    return;
                }
            }
            ret = avcodec_receive_packet(m_vEncodeCtx, &pkt);
            if (ret < 0)
            {
                av_packet_unref(&pkt);
                if (ret == AVERROR(EAGAIN))
                {
                    qDebug() << "flush video EAGAIN avcodec_receive_packet";
                    ret = 1;
                    continue;
                }
                else if (ret == AVERROR_EOF)
                {
                    qDebug() << "flush video encoder finished";
                    //break;
                    if (!(--nFlush))
                        break;
                    m_vCurPts = INT_MAX;
                    continue;
                }
                qDebug() << "flush video avcodec_receive_packet failed, ret: " << ret;
                return;
            }
            pkt.stream_index = m_vOutIndex;
            //将pts从编码层的timebase转成复用层的timebase
            av_packet_rescale_ts(&pkt, m_vEncodeCtx->time_base, m_oFmtCtx->streams[m_vOutIndex]->time_base);
            m_vCurPts = pkt.pts;
            //qDebug() << "m_vCurPts: " << m_vCurPts;

            ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
            if (ret == 0)
                qDebug() << "flush Write video packet id: " << ++g_vEncodeFrameCnt;
            else
                qDebug() << "flush video av_interleaved_write_frame failed, ret:" << ret;
            av_free_packet(&pkt);
        }
        else
        {
            if (!aBeginFlush)
            {
                aBeginFlush = true;
                ret = avcodec_send_frame(m_aEncodeCtx, nullptr);
                if (ret != 0)
                {
                    qDebug() << "flush audio avcodec_send_frame failed, ret: " << ret;
                    return;
                }
            }
            ret = avcodec_receive_packet(m_aEncodeCtx, &pkt);
            if (ret < 0)
            {
                av_packet_unref(&pkt);
                if (ret == AVERROR(EAGAIN))
                {
                    qDebug() << "flush EAGAIN avcodec_receive_packet";
                    ret = 1;
                    continue;
                }
                else if (ret == AVERROR_EOF)
                {
                    qDebug() << "flush audio encoder finished";
                    /*break;*/
                    if (!(--nFlush))
                        break;
                    m_aCurPts = INT_MAX;
                    continue;
                }
                qDebug() << "flush audio avcodec_receive_packet failed, ret: " << ret;
                return;
            }
            pkt.stream_index = m_aOutIndex;
            //将pts从编码层的timebase转成复用层的timebase
            av_packet_rescale_ts(&pkt, m_aEncodeCtx->time_base, m_oFmtCtx->streams[m_aOutIndex]->time_base);
            m_aCurPts = pkt.pts;
            qDebug() << "m_aCurPts: " << m_aCurPts;
            ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
            if (ret == 0)
                qDebug() << "flush write audio packet id: " << ++g_aEncodeFrameCnt;
            else
                qDebug() << "flush audio av_interleaved_write_frame failed, ret: " << ret;
            av_free_packet(&pkt);
        }
    }
}

void ScreenRecordImpl::Release()
{
    if (m_vOutFrame)
    {
        av_frame_free(&m_vOutFrame);
        m_vOutFrame = nullptr;
    }
    if (m_vOutFrameBuf)
    {
        av_free(m_vOutFrameBuf);
        m_vOutFrameBuf = nullptr;
    }
    if (m_oFmtCtx)
    {
        avio_close(m_oFmtCtx->pb);
        avformat_free_context(m_oFmtCtx);
        m_oFmtCtx = nullptr;
    }

    if (m_aDecodeCtx)
    {
        avcodec_free_context(&m_aDecodeCtx);
        m_aDecodeCtx = nullptr;
    }
    if (m_vEncodeCtx)
    {
        avcodec_free_context(&m_vEncodeCtx);
        m_vEncodeCtx = nullptr;
    }
    if (m_aEncodeCtx)
    {
        avcodec_free_context(&m_aEncodeCtx);
        m_aEncodeCtx = nullptr;
    }
    if (m_vFifoBuf)
    {
        av_fifo_freep(&m_vFifoBuf);
        m_vFifoBuf = nullptr;
    }
    if (m_aFifoBuf)
    {
        av_audio_fifo_free(m_aFifoBuf);
        m_aFifoBuf = nullptr;
    }
    if (m_vFmtCtx)
    {
        avformat_close_input(&m_vFmtCtx);
        m_vFmtCtx = nullptr;
    }
    if (m_aFmtCtx)
    {
        avformat_close_input(&m_aFmtCtx);
        m_aFmtCtx = nullptr;
    }

	//if (m_vDecodeCtx)
	//{
	//	// FIXME: 为什么这里会崩溃
	//	avcodec_free_context(&m_vDecodeCtx);
	//	m_vDecodeCtx = nullptr;
	//}
}

void ScreenRecordImpl::MuxThreadProc()
{
    int ret = -1;
    bool done = false;
    int vFrameIndex = 0, aFrameIndex = 0;

    av_register_all();
    avdevice_register_all();
    avcodec_register_all();

    if (OpenVideo() < 0)
        return;
    if (OpenAudio() < 0)
        return;
    if (OpenOutput() < 0)
        return;

    InitVideoBuffer();
    InitAudioBuffer();

    //启动音视频数据采集线程
    std::thread screenRecord(&ScreenRecordImpl::ScreenRecordThreadProc, this);
    std::thread soundRecord(&ScreenRecordImpl::SoundRecordThreadProc, this);
    screenRecord.detach();
    soundRecord.detach();

    while (1)
    {
        if (m_state == RecordState::Stopped && !done)
            done = true;
        if (done)
        {
            unique_lock<mutex> vBufLock(m_mtxVBuf, std::defer_lock);
            unique_lock<mutex> aBufLock(m_mtxABuf, std::defer_lock);
            std::lock(vBufLock, aBufLock);
            if (av_fifo_size(m_vFifoBuf) < m_vOutFrameSize &&
                av_audio_fifo_size(m_aFifoBuf) < m_nbSamples)
            {
                qDebug() << "both video and audio fifo buf are empty, break";
                break;
            }
        }
        if (av_compare_ts(m_vCurPts, m_oFmtCtx->streams[m_vOutIndex]->time_base,
            m_aCurPts, m_oFmtCtx->streams[m_aOutIndex]->time_base) <= 0)
            /*	if (av_compare_ts(vCurPts, m_vEncodeCtx->time_base,
                    aCurPts, m_aEncodeCtx->time_base) <= 0)*/
        {
            if (done)
            {
                lock_guard<mutex> lk(m_mtxVBuf);
                if (av_fifo_size(m_vFifoBuf) < m_vOutFrameSize)
                {
                    qDebug() << "video wirte done";
                    //break;
                    //m_vCurPts = 0x7ffffffffffffffe;	//int64_t最大有符号整数
                    m_vCurPts = INT_MAX;
                    continue;
                }
            }
            else
            {
                unique_lock<mutex> lk(m_mtxVBuf);
                m_cvVBufNotEmpty.wait(lk, [this] { return av_fifo_size(m_vFifoBuf) >= m_vOutFrameSize; });
            }
            av_fifo_generic_read(m_vFifoBuf, m_vOutFrameBuf, m_vOutFrameSize, NULL);
            m_cvVBufNotFull.notify_one();

            //设置视频帧参数
            //m_vOutFrame->pts = vFrameIndex * ((m_oFmtCtx->streams[m_vOutIndex]->time_base.den / m_oFmtCtx->streams[m_vOutIndex]->time_base.num) / m_fps);
            m_vOutFrame->pts = vFrameIndex++;
			//m_vOutFrame->pts = av_frame_get_best_effort_timestamp(m_vOutFrame);	// 这样设置pts可以测试是否可行
            m_vOutFrame->format = m_vEncodeCtx->pix_fmt;
            m_vOutFrame->width = m_vEncodeCtx->width;
            m_vOutFrame->height = m_vEncodeCtx->height;

            AVPacket pkt = { 0 };
            av_init_packet(&pkt);
            ret = avcodec_send_frame(m_vEncodeCtx, m_vOutFrame);
            if (ret != 0)
            {
                qDebug() << "video avcodec_send_frame failed, ret: " << ret;
                av_packet_unref(&pkt);
                continue;
            }
            ret = avcodec_receive_packet(m_vEncodeCtx, &pkt);
            if (ret != 0)
            {
                qDebug() << "video avcodec_receive_packet failed, ret: " << ret;
                av_packet_unref(&pkt);
                continue;
            }
            pkt.stream_index = m_vOutIndex;
            //将pts从编码层的timebase转成复用层的timebase
            av_packet_rescale_ts(&pkt, m_vEncodeCtx->time_base, m_oFmtCtx->streams[m_vOutIndex]->time_base);

            m_vCurPts = pkt.pts;
            //qDebug() << "m_vCurPts: " << m_vCurPts;

            ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
			if (ret == 0)
			{
                //qDebug() << "Write video packet id: " << ++g_vEncodeFrameCnt;
			}
            else
                qDebug() << "video av_interleaved_write_frame failed, ret:" << ret;
            av_free_packet(&pkt);
        }
        else
        {
            if (done)
            {
                lock_guard<mutex> lk(m_mtxABuf);
                if (av_audio_fifo_size(m_aFifoBuf) < m_nbSamples)
                {
                    qDebug() << "audio write done";
                    //m_aCurPts = 0x7fffffffffffffff;
                    m_aCurPts = INT_MAX;
                    continue;
                }
            }
            else
            {
                unique_lock<mutex> lk(m_mtxABuf);
                m_cvABufNotEmpty.wait(lk, [this] { return av_audio_fifo_size(m_aFifoBuf) >= m_nbSamples; });
            }

            int ret = -1;
            AVFrame *aFrame = av_frame_alloc();
            aFrame->nb_samples = m_nbSamples;
            aFrame->channel_layout = m_aEncodeCtx->channel_layout;
            aFrame->format = m_aEncodeCtx->sample_fmt;
            aFrame->sample_rate = m_aEncodeCtx->sample_rate;
            aFrame->pts = m_nbSamples * aFrameIndex++;
            //分配data buf
            ret = av_frame_get_buffer(aFrame, 0);
            av_audio_fifo_read(m_aFifoBuf, (void **)aFrame->data, m_nbSamples);
            m_cvABufNotFull.notify_one();

            AVPacket pkt = { 0 };
            av_init_packet(&pkt);
            ret = avcodec_send_frame(m_aEncodeCtx, aFrame);
            if (ret != 0)
            {
                qDebug() << "audio avcodec_send_frame failed, ret: " << ret;
                av_frame_free(&aFrame);
                av_packet_unref(&pkt);
                continue;
            }
            ret = avcodec_receive_packet(m_aEncodeCtx, &pkt);
            if (ret != 0)
            {
                qDebug() << "audio avcodec_receive_packet failed, ret: " << ret;
                av_frame_free(&aFrame);
                av_packet_unref(&pkt);
                continue;
            }
            pkt.stream_index = m_aOutIndex;

            av_packet_rescale_ts(&pkt, m_aEncodeCtx->time_base, m_oFmtCtx->streams[m_aOutIndex]->time_base);

            m_aCurPts = pkt.pts;
            //qDebug() << "aCurPts: " << m_aCurPts;

            ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
			if (ret == 0)
			{
                //qDebug() << "Write audio packet id: " << ++g_aEncodeFrameCnt;
			}
            else
                qDebug() << "audio av_interleaved_write_frame failed, ret: " << ret;

            av_frame_free(&aFrame);
            av_free_packet(&pkt);
        }
    }
    FlushEncoders();
    ret = av_write_trailer(m_oFmtCtx);
    Release();
    qDebug() << "parent thread exit";
}

void ScreenRecordImpl::ScreenRecordThreadProc()
{
    int ret = -1;
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    int y_size = m_width * m_height;
    AVFrame	*oldFrame = av_frame_alloc();
    AVFrame *newFrame = av_frame_alloc();

    int newFrameBufSize = av_image_get_buffer_size(m_vEncodeCtx->pix_fmt, m_width, m_height, 1);
    uint8_t *newFrameBuf = (uint8_t*)av_malloc(newFrameBufSize);
    av_image_fill_arrays(newFrame->data, newFrame->linesize, newFrameBuf,
        m_vEncodeCtx->pix_fmt, m_width, m_height, 1);

    while (m_state != RecordState::Stopped)
    {
        if (m_state == RecordState::Paused)
        {
            unique_lock<mutex> lk(m_mtxPause);
            m_cvNotPause.wait(lk, [this] { return m_state != RecordState::Paused; });
        }
        if (av_read_frame(m_vFmtCtx, &pkt) < 0)
        {
            qDebug() << "video av_read_frame < 0";
            continue;
        }
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
        //AVRational tb = m_vFmtCtx->streams[m_vIndex]->time_base;
        //int currentPos = pkt.pts * av_q2d(tb);

        ++g_vCollectFrameCnt;
        sws_scale(m_swsCtx, (const uint8_t* const*)oldFrame->data, oldFrame->linesize, 0,
            m_vEncodeCtx->height, newFrame->data, newFrame->linesize);

        {
            unique_lock<mutex> lk(m_mtxVBuf);
            m_cvVBufNotFull.wait(lk, [this] { return av_fifo_space(m_vFifoBuf) >= m_vOutFrameSize; });
        }
        av_fifo_generic_write(m_vFifoBuf, newFrame->data[0], y_size, NULL);
        av_fifo_generic_write(m_vFifoBuf, newFrame->data[1], y_size / 4, NULL);
        av_fifo_generic_write(m_vFifoBuf, newFrame->data[2], y_size / 4, NULL);
        m_cvVBufNotEmpty.notify_one();

        av_packet_unref(&pkt);
    }
    FlushVideoDecoder();

    av_free(newFrameBuf);
    av_frame_free(&oldFrame);
    av_frame_free(&newFrame);
    qDebug() << "screen record thread exit";
}

void ScreenRecordImpl::SoundRecordThreadProc()
{
    int ret = -1;
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    int nbSamples = m_nbSamples;
    int dstNbSamples, maxDstNbSamples;
    AVFrame *rawFrame = av_frame_alloc();
    AVFrame *newFrame = AllocAudioFrame(m_aEncodeCtx, nbSamples);

    maxDstNbSamples = dstNbSamples = av_rescale_rnd(nbSamples,
        m_aEncodeCtx->sample_rate, m_aDecodeCtx->sample_rate, AV_ROUND_UP);

    while (m_state != RecordState::Stopped)
    {
        if (m_state == RecordState::Paused)
        {
            unique_lock<mutex> lk(m_mtxPause);
            m_cvNotPause.wait(lk, [this] { return m_state != RecordState::Paused; });
        }
        if (av_read_frame(m_aFmtCtx, &pkt) < 0)
        {
            qDebug() << "audio av_read_frame < 0";
            continue;
        }
        if (pkt.stream_index != m_aIndex)
        {
            qDebug() << "not a audio packet";
            av_packet_unref(&pkt);
            continue;
        }
        ret = avcodec_send_packet(m_aDecodeCtx, &pkt);
        if (ret != 0)
        {
            qDebug() << "audio avcodec_send_packet failed, ret: " << ret;
            av_packet_unref(&pkt);
            continue;
        }
        ret = avcodec_receive_frame(m_aDecodeCtx, rawFrame);
        if (ret != 0)
        {
            qDebug() << "audio avcodec_receive_frame failed, ret: " << ret;
            av_packet_unref(&pkt);
            continue;
        }
        ++g_aCollectFrameCnt;

        qDebug() << "rawFrame->nb_samples:" << rawFrame->nb_samples;
        dstNbSamples = av_rescale_rnd(swr_get_delay(m_swrCtx, m_aDecodeCtx->sample_rate) + rawFrame->nb_samples,
            m_aEncodeCtx->sample_rate, m_aDecodeCtx->sample_rate, AV_ROUND_UP);
        if (dstNbSamples > maxDstNbSamples)
        {
            qDebug() << "audio newFrame realloc";
            av_freep(&newFrame->data[0]);
            //nb_samples*nb_channels*Bytes_sample_fmt
            ret = av_samples_alloc(newFrame->data, newFrame->linesize, m_aEncodeCtx->channels,
                dstNbSamples, m_aEncodeCtx->sample_fmt, 1);
            if (ret < 0)
            {
                qDebug() << "av_samples_alloc failed";
                return;
            }

            maxDstNbSamples = dstNbSamples;
            m_aEncodeCtx->frame_size = dstNbSamples;
            m_nbSamples = newFrame->nb_samples;	//1024
        }

        newFrame->nb_samples = swr_convert(m_swrCtx, newFrame->data, dstNbSamples,
            (const uint8_t **)rawFrame->data, rawFrame->nb_samples);
        if (newFrame->nb_samples < 0)
        {
            qDebug() << "swr_convert error";
            return;
        }
        {
            unique_lock<mutex> lk(m_mtxABuf);
            m_cvABufNotFull.wait(lk, [newFrame, this] { return av_audio_fifo_space(m_aFifoBuf) >= newFrame->nb_samples; });
        }
        if (av_audio_fifo_write(m_aFifoBuf, (void **)newFrame->data, newFrame->nb_samples) < newFrame->nb_samples)
        {
            qDebug() << "av_audio_fifo_write";
            return;
        }
        m_cvABufNotEmpty.notify_one();
    }
    FlushAudioDecoder();
    av_frame_free(&rawFrame);
    av_frame_free(&newFrame);
    qDebug() << "sound record thread exit";
}
