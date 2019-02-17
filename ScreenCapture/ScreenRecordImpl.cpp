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
		vStream->time_base.num = 1;
		vStream->time_base.den = m_fps;

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

		//查找视频编码器
		m_vEncodeCtx->codec = avcodec_find_encoder(m_vEncodeCtx->codec_id);
		if (!m_vEncodeCtx->codec)
		{
			qDebug() << "Can not find the encoder, id: " << m_vEncodeCtx->codec_id;
			return -1;
		}
		m_vEncodeCtx->codec_tag = 0;
		//正确设置sps/pps
		m_vEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		//打开视频编码器
		if ((avcodec_open2(m_vEncodeCtx, m_vEncodeCtx->codec, NULL)) < 0)
		{
			printf("can not open the encoder\n");
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
		m_aEncodeCtx = avcodec_alloc_context3(NULL);
		if (nullptr == m_vEncodeCtx)
		{
			qDebug() << "audio avcodec_alloc_context3 failed";
			return -1;
		}
		m_aEncodeCtx->codec = avcodec_find_encoder(m_oFmtCtx->oformat->audio_codec);
		if (!m_aEncodeCtx->codec)
		{
			qDebug() << "Can not find audio encoder, id: " << m_oFmtCtx->oformat->audio_codec;
			return -1;
		}

		ret = avcodec_parameters_to_context(m_aEncodeCtx, m_aFmtCtx->streams[m_aIndex]->codecpar);
		if (ret < 0)
		{
			qDebug() << "Output audio avcodec_parameters_to_context,error code:" << ret;
			return -1;
		}
		aStream->time_base = AVRational{ 1, m_aFmtCtx->streams[m_aIndex]->codecpar->sample_rate };

		m_aEncodeCtx->codec_tag = 0;
		m_aEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		//打开音频编码器
		if (avcodec_open2(m_aEncodeCtx, m_aEncodeCtx->codec, 0) < 0)
		{
			qDebug() << "Can not open the audio encoder, id: " << m_aFmtCtx->streams[m_aIndex]->codecpar->codec_id;
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

void ScreenRecordImpl::MuxThreadProc()
{
	av_register_all();
	avdevice_register_all();
	if (OpenVideo() < 0)
		return;
	if (OpenAudio() < 0)
		return;
	if (OpenOutput() < 0)
		return;

	AVFrame *picture = av_frame_alloc();
	av_image_get_buffer_size(m_vEncodeCtx->pix_fmt, m_width, m_height, 1);


	int size = avpicture_get_size(pFormatCtx_Out->streams[VideoIndex]->codec->pix_fmt,
		pFormatCtx_Out->streams[VideoIndex]->codec->width, pFormatCtx_Out->streams[VideoIndex]->codec->height);
	picture_buf = new uint8_t[size];

	avpicture_fill((AVPicture *)picture, picture_buf,
		pFormatCtx_Out->streams[VideoIndex]->codec->pix_fmt,
		pFormatCtx_Out->streams[VideoIndex]->codec->width,
		pFormatCtx_Out->streams[VideoIndex]->codec->height);



	//star cap screen thread
	CreateThread(NULL, 0, ScreenCapThreadProc, 0, 0, NULL);
	//star cap audio thread
	CreateThread(NULL, 0, AudioCapThreadProc, 0, 0, NULL);
	int64_t cur_pts_v = 0, cur_pts_a = 0;
	int VideoFrameIndex = 0, AudioFrameIndex = 0;


}
