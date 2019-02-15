#include "ScreenRecordImpl.h"

#include <Windows.h>

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

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "swscale.lib")
#ifdef __cplusplus
};
#endif

ScreenRecordImpl::ScreenRecordImpl() :
	m_fps(25)
	, m_vIndex(-1), m_aIndex(-1)
	, m_vFmtCtx(nullptr), m_aFmtCtx(nullptr), m_oFmtCtx(nullptr)
	, m_vCodecCtx(nullptr), m_aCodecCtx(nullptr)
	, m_vCodec(nullptr), m_aCodec(nullptr)
	, m_vBuf(nullptr), m_aBuf(nullptr)
	, m_swsCtx(nullptr)
{
}

int ScreenRecordImpl::OpenVideo()
{
	int ret = -1;
	AVInputFormat *ifmt = av_find_input_format("gdigrab");
	AVDictionary *options = nullptr;
	av_dict_set(&options, "framerate", QString::number(m_fps).toStdString().c_str(), NULL);

	if (avformat_open_input(&m_vFmtCtx, "desktop", ifmt, &options) != 0)
	{
		printf("Couldn't open input stream.（无法打开视频输入流）\n");
		return -1;
	}
	if (avformat_find_stream_info(m_vFmtCtx, nullptr) < 0)
	{
		printf("Couldn't find stream information.（无法获取视频流信息）\n");
		return -1;
	}
	for (int i = 0; i < m_vFmtCtx->nb_streams; ++i)
	{
		if (m_vFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			//从视频流中拷贝参数到codecCtx
			m_vCodecCtx = avcodec_alloc_context3(nullptr);
			if ((ret = avcodec_parameters_to_context(m_vCodecCtx, m_vFmtCtx->streams[i]->codecpar)) < 0)
			{
				qDebug() << "Video avcodec_parameters_to_context failed,error code: " << ret;
				return -1;
			}
			m_vIndex = i;
			break;
		}
	}
	m_vCodec = avcodec_find_decoder(m_vCodecCtx->codec_id);
	if (m_vCodec == nullptr)
	{
		printf("Codec not found.（没有找到解码器）\n");
		return -1;
	}
	if (avcodec_open2(m_vCodecCtx, m_vCodec, nullptr) < 0)
	{
		printf("Could not open codec.（无法打开解码器）\n");
		return -1;
	}

	m_swsCtx = sws_getContext(m_vCodecCtx->width, m_vCodecCtx->height, m_vCodecCtx->pix_fmt,
		m_vCodecCtx->width, m_vCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

	//frame_size = avpicture_get_size(pCodecCtx_Video->pix_fmt, pCodecCtx_Video->width, pCodecCtx_Video->height);
	//申请30帧缓存,width*height*1.5
	int bufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_vCodecCtx->width, m_vCodecCtx->height, 1);
	m_vBuf = av_fifo_alloc(30 * bufSize);

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

int ScreenRecordImpl::OpenAudio()
{
	int ret = -1;
	//查找输入方式
	AVInputFormat *ifmt = av_find_input_format("dshow");

	//以Direct Show的方式打开设备，并将 输入方式 关联到格式上下文
	char * deviceName = dup_wchar_to_utf8(L"audio=麦克风 (Realtek High Definition Au");

	if (avformat_open_input(&m_aFmtCtx, deviceName, ifmt, nullptr) < 0)
	{
		printf("Couldn't open input stream.（无法打开音频输入流）\n");
		return -1;
	}

	if (avformat_find_stream_info(m_aFmtCtx, nullptr) < 0)
		return -1;

	for (int i = 0; i < m_aFmtCtx->nb_streams; ++i)
	{
		if (m_aFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			//从视频流中拷贝参数到codecCtx
			m_aCodecCtx = avcodec_alloc_context3(nullptr);
			if ((ret = avcodec_parameters_to_context(m_aCodecCtx, m_aFmtCtx->streams[i]->codecpar)) < 0)
			{
				qDebug() << "Audio avcodec_parameters_to_context failed,error code: " << ret;
				return -1;
			}
			m_aIndex = i;
			break;
		}
	}

	m_aCodec = avcodec_find_decoder(m_aCodecCtx->codec_id);
	if (m_aCodec == nullptr)
	{
		printf("Codec not found.（没有找到解码器）\n");
		return -1;
	}
	if (0 > avcodec_open2(m_aCodecCtx, m_aCodec, NULL))
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
	const char *outFileName = "test.mp4";
	avformat_alloc_output_context2(&m_oFmtCtx, nullptr, nullptr, outFileName);

	if (m_vFmtCtx->streams[m_vIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		//AVCodecContext *videoCodecCtx;
		//VideoIndex = 0;
		vStream = avformat_new_stream(m_oFmtCtx, nullptr);
		if (!vStream)
		{
			printf("can not new stream for output!\n");
			return -1;
		}

		vStream->time_base.num = 1;
		vStream->time_base.den = m_fps;
		m_vCodecCtx->codec_tag = 0;
		if (m_vFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
			m_vFmtCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		////将codecCtx中的参数传给输出流
		//if ((ret = avcodec_parameters_from_context(vStream->codecpar, m_vCodecCtx)) < 0) {
		//	qDebug() << "Output avcodec_parameters_from_context,error code:" << ret;
		//	return -1;
		//}

		m_vEncCodecCtx = avcodec_alloc_context3(NULL);
		if (nullptr == m_vEncCodecCtx)
		{
			qDebug() << "avcodec_alloc_context3 failed";
			return -1;
		}
		m_vEncCodecCtx->width = m_width;
		m_vEncCodecCtx->height = m_height;
		m_vEncCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
		m_vEncCodecCtx->time_base.num = 1;
		m_vEncCodecCtx->time_base.den = m_fps;	

		//vStream->codec->codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		vStream->codec->height = pFormatCtx_Video->streams[0]->codec->height;
		vStream->codec->width = pFormatCtx_Video->streams[0]->codec->width;

		vStream->codec->time_base = pFormatCtx_Video->streams[0]->codec->time_base;
		vStream->codec->sample_aspect_ratio = pFormatCtx_Video->streams[0]->codec->sample_aspect_ratio;
		// take first format from list of supported formats
		vStream->codec->pix_fmt = pFormatCtx_Out->streams[VideoIndex]->codec->codec->pix_fmts[0];

		//open encoder
		if (!vStream->codec->codec)
		{
			printf("can not find the encoder!\n");
			return -1;
		}

		if (pFormatCtx_Out->oformat->flags & AVFMT_GLOBALHEADER)
			vStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		if ((avcodec_open2(vStream->codec, vStream->codec->codec, NULL)) < 0)
		{
			printf("can not open the encoder\n");
			return -1;
		}
	}

	if (pFormatCtx_Audio->streams[0]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		AVCodecContext *pOutputCodecCtx;
		AudioIndex = 1;
		aStream = avformat_new_stream(pFormatCtx_Out, NULL);

		aStream->codec->codec = avcodec_find_encoder(pFormatCtx_Out->oformat->audio_codec);

		pOutputCodecCtx = aStream->codec;

		pOutputCodecCtx->sample_rate = pFormatCtx_Audio->streams[0]->codec->sample_rate;
		pOutputCodecCtx->channel_layout = pFormatCtx_Out->streams[0]->codec->channel_layout;
		pOutputCodecCtx->channels = av_get_channel_layout_nb_channels(aStream->codec->channel_layout);
		if (pOutputCodecCtx->channel_layout == 0)
		{
			pOutputCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
			pOutputCodecCtx->channels = av_get_channel_layout_nb_channels(pOutputCodecCtx->channel_layout);

		}
		pOutputCodecCtx->sample_fmt = aStream->codec->codec->sample_fmts[0];
		AVRational time_base = { 1, aStream->codec->sample_rate };
		aStream->time_base = time_base;
		//audioCodecCtx->time_base = time_base;

		pOutputCodecCtx->codec_tag = 0;
		if (pFormatCtx_Out->oformat->flags & AVFMT_GLOBALHEADER)
			pOutputCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

		if (avcodec_open2(pOutputCodecCtx, pOutputCodecCtx->codec, 0) < 0)
		{
			//编码器打开失败，退出程序
			return -1;
		}
	}


	if (!(pFormatCtx_Out->oformat->flags & AVFMT_NOFILE))
	{
		if (avio_open(&pFormatCtx_Out->pb, outFileName, AVIO_FLAG_WRITE) < 0)
		{
			printf("can not open output file handle!\n");
			return -1;
		}
	}

	if (avformat_write_header(pFormatCtx_Out, NULL) < 0)
	{
		printf("can not write the header of the output file!\n");
		return -1;
	}

	return 0;
}


