
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
//#pragma comment(lib, "avcodec.lib")
//#pragma comment(lib, "avformat.lib")
//#pragma comment(lib, "avutil.lib")
//#pragma comment(lib, "avdevice.lib")
//#pragma comment(lib, "avfilter.lib")
//#pragma comment(lib, "swscale.lib")

#ifdef __cplusplus
};
#endif

#include "ScreenRecordImpl.h"
#include <QDebug>
#include <QAudioDeviceInfo>
#include <thread>

#include <dshow.h>

using namespace std;

ScreenRecordImpl::ScreenRecordImpl(QObject * parent) :
	QObject(parent)
	, m_fps(25)
	, m_vIndex(-1), m_aIndex(-1)
	, m_vFmtCtx(nullptr), m_aFmtCtx(nullptr), m_oFmtCtx(nullptr)
	, m_vDecodeCtx(nullptr), m_aDecodeCtx(nullptr)
	, m_vBuf(nullptr), m_aBuf(nullptr)
	, m_swsCtx(nullptr)
	, m_stop(false)
	, m_vFrameIndex(0), m_aFrameIndex(0)
	, m_started(false)
{
}

void ScreenRecordImpl::Start()
{
	if (!m_started)
	{
		m_started = true;
		m_filePath = "test.mp4";
		m_width = 1920;
		m_height = 1080;
		m_fps = 30;
		std::thread muxThread(&ScreenRecordImpl::MuxThreadProc, this);
		muxThread.detach();
	}
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
		AVStream *stream = m_vFmtCtx->streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			decoder = avcodec_find_decoder(stream->codecpar->codec_id);
			if (decoder == nullptr)
			{
				printf("Codec not found.（没有找到解码器）\n");
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
		m_width, m_height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

	//申请30帧缓存,width*height*1.5
	m_videoFrameSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_width, m_height, 1);
	m_vBuf = av_fifo_alloc(30 * m_videoFrameSize);
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
	AVCodec *decoder = nullptr;

	qDebug() << GetSpeakerDeviceName();
	const char *speaker = GetSpeakerDeviceName().toStdString().c_str();
	//AVDictionary* options = NULL;
	//av_dict_set(&options, "list_devices", "true", 0);
	//AVInputFormat *iformat = av_find_input_format("dshow");
	//printf("Device Info=============");
	//avformat_open_input(&m_aFmtCtx, "audio=dummy", iformat, &options);
	//printf("========================");
	qDebug() << GetMicrophoneDeviceName();


	//查找输入方式
	AVInputFormat *ifmt = av_find_input_format("dshow");
	//以Direct Show的方式打开设备，并将 输入方式 关联到格式上下文
	char * deviceName = dup_wchar_to_utf8(L"audio=麦克风 (Conexant SmartAudio HD)");
	//char * deviceName = dup_wchar_to_utf8(L"DirectSound: 扬声器(Conexant SmartAudio HD)");
	//char * deviceName = dup_wchar_to_utf8(L"audio=扬声器 (Conexant SmartAudio HD)");
	//char * deviceName = dup_wchar_to_utf8(L"audio=DirectSound_ 扬声器 (Conexant SmartAudio HD)");
	if (avformat_open_input(&m_aFmtCtx, deviceName, ifmt, nullptr) < 0)
	{
		printf("Couldn't open input stream.（无法打开音频输入流）\n");
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
			//从视频流中拷贝参数到codecCtx
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
		//AVFormatContext第一个创建的流索引是0，第二个创建的流索引是1
		m_vOutIndex = vStream->index;
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

		m_vEncodeCtx->bit_rate = 800 * 1000;
		m_vEncodeCtx->rc_max_rate = 800 * 1000;
		//codec_ctx->rc_min_rate = 200 * 1000;
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
		aStream->time_base = AVRational{ 1, m_aFmtCtx->streams[m_aIndex]->codecpar->sample_rate };
		m_aOutIndex = aStream->index;

		//AVCodec *encoder = avcodec_find_encoder(m_oFmtCtx->oformat->audio_codec);
		AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_MP3);
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
		//ret = avcodec_parameters_to_context(m_aEncodeCtx, m_aFmtCtx->streams[m_aIndex]->codecpar);
		//if (ret < 0)
		//{
		//	qDebug() << "Output audio avcodec_parameters_to_context,error code:" << ret;
		//	return -1;
		//}
		m_aEncodeCtx->sample_rate = 44100;
		m_aEncodeCtx->channel_layout = AV_CH_LAYOUT_STEREO;
		m_aEncodeCtx->channels = 2;
		m_aEncodeCtx->codec_type = AVMEDIA_TYPE_AUDIO;
		m_aEncodeCtx->sample_fmt = AV_SAMPLE_FMT_S16P;
		m_aEncodeCtx->codec_id = AV_CODEC_ID_MP3;
		m_aEncodeCtx->codec_tag = 0;
		m_aEncodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		//打开音频编码器
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

void ScreenRecordImpl::MuxThreadProc()
{
	int ret = -1;
	bool done = false;

	av_register_all();
	avdevice_register_all();
	avcodec_register_all();
	if (OpenVideo() < 0)
		return;
	if (OpenAudio() < 0)
		return;
	if (OpenOutput() < 0)
		return;

	InitializeCriticalSection(&m_vSection);
	InitializeCriticalSection(&m_aSection);

	m_vOutFrame = av_frame_alloc();
	m_vOutFrameSize = av_image_get_buffer_size(m_vEncodeCtx->pix_fmt, m_width, m_height, 1);
	m_vOutFrameBuf = (uint8_t *)av_malloc(m_vOutFrameSize);
	//先让AVFrame指针指向buf，后面再写入数据到buf
	av_image_fill_arrays(m_vOutFrame->data, m_vOutFrame->linesize, m_vOutFrameBuf, m_vEncodeCtx->pix_fmt, m_width, m_height, 1);

	m_aOutFrame = av_frame_alloc();
	AVStream *aOutStream = m_oFmtCtx->streams[m_aIndex];
	int aOutFrameSize = av_samples_get_buffer_size(nullptr, aOutStream->codecpar->channels,
		aOutStream->codecpar->frame_size, (AVSampleFormat)aOutStream->codecpar->format, 1);
	m_aOutFrameBuf = (uint8_t *)av_malloc(aOutFrameSize);

	m_aOutFrame->nb_samples = m_aEncodeCtx->frame_size;
	m_aOutFrame->format = m_aEncodeCtx->sample_fmt;

	//让音频帧指向buf，调用之前要设置nb_samples
	avcodec_fill_audio_frame(m_aOutFrame, aOutStream->codecpar->channels,
		(AVSampleFormat)aOutStream->codecpar->format, (const uint8_t*)m_aOutFrameBuf, aOutFrameSize, 1);

	std::thread screenRecord(&ScreenRecordImpl::ScreenRecordThreadProc, this);
	std::thread soundRecord(&ScreenRecordImpl::SoundRecordThreadProc, this);

	int64_t vCurPts = 0, aCurPts = 0;
	int vFrameIndex = 0, aFrameIndex = 0;

	while (1)
	{
		if (m_stop)
		{
			done = true;
		}
		if (m_vBuf && m_aBuf)
		{
			//缓存数据写完就结束循环
			if (done && av_fifo_size(m_vBuf) <= m_videoFrameSize &&
				av_audio_fifo_size(m_aBuf) <= m_vEncodeCtx->frame_size)
				break;
		}

		if (av_compare_ts(vCurPts, m_oFmtCtx->streams[m_vIndex]->time_base,
			aCurPts, m_oFmtCtx->streams[m_aIndex]->time_base) <= 0)
		{
			//read data from fifo
			if (done && av_fifo_size(m_vBuf) < m_videoFrameSize)
				vCurPts = 0x7fffffffffffffff;

			if (av_fifo_size(m_vBuf) >= m_vOutFrameSize)
			{
				EnterCriticalSection(&m_vSection);
				av_fifo_generic_read(m_vBuf, m_vOutFrameBuf, m_vOutFrameSize, NULL);
				LeaveCriticalSection(&m_vSection);

				//调用一次就可以了吧？
				//av_image_fill_arrays(m_vOutFrame->data, m_vOutFrame->linesize, m_vOutFrameBuf, m_vEncodeCtx->pix_fmt, m_width, m_height, 1);

				//设置视频帧参数
				m_vOutFrame->pts = vFrameIndex * ((m_oFmtCtx->streams[m_vIndex]->time_base.den / m_oFmtCtx->streams[m_vIndex]->time_base.num) / m_fps);
				++vFrameIndex;
				m_vOutFrame->format = m_vEncodeCtx->pix_fmt;
				m_vOutFrame->width = m_width;
				m_vOutFrame->height = m_height;

				AVPacket pkt;
				av_new_packet(&pkt, m_vOutFrameSize);
				ret = avcodec_send_frame(m_vEncodeCtx, m_vOutFrame);
				if (ret != 0)
				{
					av_packet_unref(&pkt);
					continue;
				}
				ret = avcodec_receive_packet(m_vEncodeCtx, &pkt);
				{
					av_packet_unref(&pkt);
					continue;
				}
				pkt.stream_index = m_vIndex;
				pkt.pts = av_rescale_q_rnd(pkt.pts, m_vFmtCtx->streams[m_vIndex]->time_base,
					m_oFmtCtx->streams[m_vIndex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				pkt.dts = av_rescale_q_rnd(pkt.dts, m_vFmtCtx->streams[m_vIndex]->time_base,
					m_oFmtCtx->streams[m_vIndex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				pkt.duration = ((m_oFmtCtx->streams[m_vIndex]->time_base.den / m_oFmtCtx->streams[m_vIndex]->time_base.num) / m_fps);
				vCurPts = pkt.pts;

				ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
				if (ret < 0)
				{
					qDebug() << "av_interleaved_write_frame failed";
				}
				av_free_packet(&pkt);
			}
		}
		else
		{
			if (nullptr == m_aBuf)
				continue;//还未初始化fifo

			if (done && av_audio_fifo_size(m_aBuf) < aOutFrameSize)
				vCurPts = 0x7fffffffffffffff;

			if (av_audio_fifo_size(m_aBuf) >=
				(aOutFrameSize > 0 ? aOutFrameSize : 1024))
			{
				EnterCriticalSection(&m_aSection);
				av_audio_fifo_read(m_aBuf, (void **)m_aOutFrame->data, (aOutFrameSize > 0 ? aOutFrameSize : 1024));
				LeaveCriticalSection(&m_aSection);

				m_aOutFrame->nb_samples = aOutFrameSize > 0 ? aOutFrameSize : 1024;
				m_aOutFrame->channel_layout = m_oFmtCtx->streams[m_aIndex]->codecpar->channel_layout;
				m_aOutFrame->format = m_oFmtCtx->streams[m_aIndex]->codecpar->format;
				m_aOutFrame->sample_rate = m_oFmtCtx->streams[m_aIndex]->codecpar->sample_rate;
				m_aOutFrame->pts = aFrameIndex * aOutFrameSize;
				++aFrameIndex;
				//av_frame_get_buffer(frame, 0);

				if (m_oFmtCtx->streams[m_aIndex]->codecpar->format != m_aFmtCtx->streams[m_aIndex]->codecpar->format
					|| m_oFmtCtx->streams[m_aIndex]->codecpar->channels != m_aFmtCtx->streams[m_aIndex]->codecpar->channels
					|| m_oFmtCtx->streams[m_aIndex]->codecpar->sample_rate != m_aFmtCtx->streams[m_aIndex]->codecpar->sample_rate)
				{
					//如果输入和输出的音频格式不一样 需要重采样，这里是一样的就没做
				}

				AVPacket pkt;
				av_new_packet(&pkt, aOutFrameSize);
				ret = avcodec_send_frame(m_aEncodeCtx, m_aOutFrame);
				if (ret != 0)
				{
					av_packet_unref(&pkt);
					continue;
				}
				ret = avcodec_receive_packet(m_aEncodeCtx, &pkt);
				{
					av_packet_unref(&pkt);
					continue;
				}
				pkt.stream_index = m_aIndex;
				pkt.pts = m_aOutFrame->pts;
				pkt.dts = pkt.pts;
				pkt.duration = aOutFrameSize;
				aCurPts = pkt.pts;

				int ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
				if (ret < 0)
				{
					qDebug() << "audio av_interleaved_write_frame failed";
				}
				av_free_packet(&pkt);
			}
		}
	}

	av_write_trailer(m_oFmtCtx);

	av_frame_free(&m_vOutFrame);
	av_frame_free(&m_aOutFrame);
	av_free(m_vOutFrameBuf);
	av_free(m_aOutFrameBuf);

	av_fifo_free(m_vBuf);
	av_audio_fifo_free(m_aBuf);

	avio_close(m_oFmtCtx->pb);
	avformat_free_context(m_oFmtCtx);

	if (m_vFmtCtx != nullptr)
	{
		avformat_close_input(&m_vFmtCtx);
		m_vFmtCtx = nullptr;
	}
	if (m_aFmtCtx != nullptr)
	{
		avformat_close_input(&m_aFmtCtx);
		m_aFmtCtx = nullptr;
	}
}

void ScreenRecordImpl::ScreenRecordThreadProc()
{
	int ret = -1;
	AVPacket pkg;
	AVFrame	*oldFrame = av_frame_alloc();
	AVFrame *newFrame = av_frame_alloc();

	av_image_fill_arrays(newFrame->data, newFrame->linesize, m_vOutFrameBuf, m_vEncodeCtx->pix_fmt, m_width, m_height, 1);
	av_new_packet(&pkg, m_vOutFrameSize);
	int y_size = m_width * m_height;

	while (!m_stop)
	{
		if (av_read_frame(m_vFmtCtx, &pkg) < 0)
			continue;
		if (pkg.stream_index == m_vIndex)
		{
			ret = avcodec_send_packet(m_vDecodeCtx, &pkg);
			if (ret != 0)
			{
				av_packet_unref(&pkg);
				continue;
			}
			ret = avcodec_receive_frame(m_vDecodeCtx, oldFrame);
			if (ret != 0)
			{
				av_packet_unref(&pkg);
				continue;
			}
			sws_scale(m_swsCtx, (const uint8_t* const*)oldFrame->data, oldFrame->linesize, 0,
				m_height, newFrame->data, newFrame->linesize);

			if (av_fifo_space(m_vBuf) >= m_vOutFrameSize)
			{
				EnterCriticalSection(&m_vSection);
				av_fifo_generic_write(m_vBuf, newFrame->data[0], y_size, NULL);
				av_fifo_generic_write(m_vBuf, newFrame->data[1], y_size / 4, NULL);
				av_fifo_generic_write(m_vBuf, newFrame->data[2], y_size / 4, NULL);
				LeaveCriticalSection(&m_vSection);
			}
		}
		av_packet_unref(&pkg);
	}
	av_frame_free(&oldFrame);
	av_frame_free(&newFrame);
}

void ScreenRecordImpl::SoundRecordThreadProc()
{
	int ret = -1;
	AVPacket pkg;
	AVFrame *frame = av_frame_alloc();
	av_new_packet(&pkg, m_aOutFrameSize);

	while (!m_stop)
	{
		if (av_read_frame(m_aFmtCtx, &pkg) < 0)
			continue;
		ret = avcodec_send_packet(m_aDecodeCtx, &pkg);
		if (ret != 0)
		{
			av_packet_unref(&pkg);
			continue;
		}
		ret = avcodec_receive_frame(m_aDecodeCtx, frame);
		if (ret != 0)
		{
			av_packet_unref(&pkg);
			continue;
		}

		if (nullptr == m_aBuf)
			m_aBuf = av_audio_fifo_alloc(m_aDecodeCtx->sample_fmt, m_aDecodeCtx->channels, 30 * frame->nb_samples);

		if (av_audio_fifo_space(m_aBuf) >= frame->nb_samples)
		{
			EnterCriticalSection(&m_aSection);
			av_audio_fifo_write(m_aBuf, (void **)frame->data, frame->nb_samples);
			LeaveCriticalSection(&m_aSection);
		}
	}
	av_frame_free(&frame);
}
