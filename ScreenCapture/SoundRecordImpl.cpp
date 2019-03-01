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

#include "SoundRecordImpl.h"
#include <QDebug>
#include <QAudioDeviceInfo>
#include <thread>
#include <fstream>

#include <dshow.h>

using namespace std;

//g_collectFrameCnt等于g_encodeFrameCnt证明编解码帧数一致
int g_collectFrameCnt = 0;	//采集帧数
int g_encodeFrameCnt = 0;	//编码帧数

ScreenRecordImpl::ScreenRecordImpl(QObject * parent) :
	QObject(parent)
	, m_aIndex(-1)
	, m_aFmtCtx(nullptr), m_oFmtCtx(nullptr)
	, m_aDecodeCtx(nullptr)
	, m_aFifoBuf(nullptr)
	, m_stop(false)
{
}

void ScreenRecordImpl::Start()
{
		m_filePath = "test.mp3";
		std::thread recordThread(&ScreenRecordImpl::RecordAudioThreadProc, this);
		recordThread.detach();
}

void ScreenRecordImpl::Pause()
{
}


void ScreenRecordImpl::Stop()
{
	m_stop = true;
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
	char * deviceName = dup_wchar_to_utf8(L"audio=麦克风 (Conexant SmartAudio HD)"); 
	//char * deviceName = dup_wchar_to_utf8(L"audio=麦克风 (High Definition Audio 设备)");
	//char * deviceName = "audio=virtual-audio-capturer";
	if (avformat_open_input(&m_aFmtCtx, deviceName, ifmt, nullptr) < 0)
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
	const char *outFileName = "test.mp3";
	ret = avformat_alloc_output_context2(&m_oFmtCtx, nullptr, nullptr, outFileName);
	if (ret < 0)
	{
		qDebug() << "avformat_alloc_output_context2 failed";
		return -1;
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
		if (nullptr == m_aEncodeCtx)
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
		m_aEncodeCtx->sample_fmt = encoder->sample_fmts ? encoder->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		m_aEncodeCtx->bit_rate = 64000;
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
	frame->channel_layout = c->channel_layout ? c->channel_layout: AV_CH_LAYOUT_STEREO;
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

void ScreenRecordImpl::FlushEncoder()
{
	int ret = -1;
	AVPacket pkt = { 0 };
	av_init_packet(&pkt);
	ret = avcodec_send_frame(m_aEncodeCtx, nullptr);
	qDebug() << "flush audio avcodec_send_frame ret: " << ret;

	while (ret >= 0)
	{
		qDebug() << "flush";
		ret = avcodec_receive_packet(m_aEncodeCtx, &pkt);
		if (ret < 0)
		{
			av_packet_unref(&pkt);
			if (ret == AVERROR(EAGAIN))
			{
				qDebug() << "flush EAGAIN avcodec_receive_packet";
				continue;
			}
			else if (ret == AVERROR_EOF)
			{
				qDebug() << "flush video encoder finished";
				break;
			}
			qDebug() << "flush audio avcodec_receive_packet failed, ret: " << ret;
			return;
		}
	}
	pkt.stream_index = m_aOutIndex;

	ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
	if (ret == 0)
		qDebug() << "flush write audio packet id: " << ++g_encodeFrameCnt;
	else
		qDebug() << "flush audio av_interleaved_write_frame failed, ret: " << ret;

	av_free_packet(&pkt);
}

void ScreenRecordImpl::RecordAudioThreadProc()
{
	int ret = -1;
	bool done = false;
	int aFrameIndex = 0;

	av_register_all();
	avdevice_register_all();
	avcodec_register_all();

	if (OpenAudio() < 0)
		return;
	if (OpenOutput() < 0)
		return;

	//1152
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

	//启动音视频数据采集线程
	std::thread soundRecord(&ScreenRecordImpl::AcquireSoundThreadProc, this);
	soundRecord.detach();

	while (1)
	{
		if (m_stop)
			done = true;
		if (done)
		{
			lock_guard<mutex> lk(m_mtx);
			if (av_audio_fifo_size(m_aFifoBuf) < m_nbSamples)
				break;
		}
		{
			std::unique_lock<mutex> lk(m_mtx);
			m_cvNotEmpty.wait(lk, [this] {return av_audio_fifo_size(m_aFifoBuf) >= m_nbSamples; });
		}
		int ret = -1;
		AVFrame *aFrame = av_frame_alloc();
		aFrame->nb_samples = m_nbSamples;
		aFrame->channel_layout = m_aEncodeCtx->channel_layout;
		aFrame->format = m_aEncodeCtx->sample_fmt;
		aFrame->sample_rate = m_aEncodeCtx->sample_rate;
		aFrame->pts = aFrameIndex * m_nbSamples;
		++aFrameIndex;
		//分配data buf
		ret = av_frame_get_buffer(aFrame, 0);

		av_audio_fifo_read(m_aFifoBuf, (void **)aFrame->data, m_nbSamples);
		m_cvNotFull.notify_one();

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
			qDebug() << "audio avcodec_receive_packet failed";
			av_frame_free(&aFrame);
			av_packet_unref(&pkt);
			continue;
		}
		pkt.stream_index = m_aOutIndex;
		//pkt.pts = aFrame->pts;
		//pkt.dts = pkt.pts;
		//pkt.duration = m_nbSamples;

		ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
		if (ret == 0)
			qDebug() << "Write audio packet id: " << ++g_encodeFrameCnt;
		else
			qDebug() << "audio av_interleaved_write_frame failed, ret: " << ret;

		av_frame_free(&aFrame);
		av_free_packet(&pkt);
	}
	FlushEncoder();

	av_write_trailer(m_oFmtCtx);

	avio_close(m_oFmtCtx->pb);
	avformat_free_context(m_oFmtCtx);

	if (m_aDecodeCtx)
	{
		avcodec_free_context(&m_aDecodeCtx);
		m_aDecodeCtx = nullptr;
	}
	if (m_aEncodeCtx)
	{
		avcodec_free_context(&m_aEncodeCtx);
		m_aEncodeCtx = nullptr;
	}
	if (m_aFifoBuf)
	{
		av_audio_fifo_free(m_aFifoBuf);
		m_aFifoBuf = nullptr;
	}
	if (m_aFmtCtx)
	{
		avformat_close_input(&m_aFmtCtx);
		m_aFmtCtx = nullptr;
	}
	qDebug() << "parent thread exit";
}

void ScreenRecordImpl::AcquireSoundThreadProc()
{
	int ret = -1;
	AVPacket pkg = { 0 };
	av_init_packet(&pkg);
	int nbSamples = m_nbSamples;
	int dstNbSamples, maxDstNbSamples;
	AVFrame *rawFrame = av_frame_alloc();
	AVFrame *newFrame = AllocAudioFrame(m_aEncodeCtx, nbSamples);

	maxDstNbSamples = dstNbSamples = av_rescale_rnd(nbSamples, 
		m_aEncodeCtx->sample_rate, m_aDecodeCtx->sample_rate, AV_ROUND_UP);

	while (!m_stop)
	{
		if (av_read_frame(m_aFmtCtx, &pkg) < 0)
		{
			qDebug() << "audio av_read_frame < 0";
			continue;
		}
		if (pkg.stream_index != m_aIndex)
		{
			av_packet_unref(&pkg);
			continue;
		}
		ret = avcodec_send_packet(m_aDecodeCtx, &pkg);
		if (ret != 0)
		{
			av_packet_unref(&pkg);
			continue;
		}
		ret = avcodec_receive_frame(m_aDecodeCtx, rawFrame);
		if (ret != 0)
		{
			av_packet_unref(&pkg);
			continue;
		}
		//qDebug() << "rawFrame nbSamples: " << rawFrame->nb_samples;
		//1152->22050
		//nbSamples换rawFrame->nb_samples
		dstNbSamples = av_rescale_rnd(swr_get_delay(m_swrCtx, m_aDecodeCtx->sample_rate) + rawFrame->nb_samples,
			m_aEncodeCtx->sample_rate, m_aDecodeCtx->sample_rate, AV_ROUND_UP);
		if (dstNbSamples > maxDstNbSamples)
		{
			qDebug() << ">>>";
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
			m_nbSamples = newFrame->nb_samples;
		}
		//dstNbSamples = av_rescale_rnd(swr_get_delay(m_swrCtx, m_aDecodeCtx->sample_rate) + rawFrame->nb_samples,
		//	m_aEncodeCtx->sample_rate, m_aDecodeCtx->sample_rate, AV_ROUND_UP);
		//av_assert0(dstNbSamples == rawFrame->nb_samples);
		//ret = av_frame_make_writable(rawFrame);
		//if (ret != 0)
		//{
		//	qDebug() << "av_frame_make_writable failed";
		//	return;
		//}

		newFrame->nb_samples = swr_convert(m_swrCtx, newFrame->data, dstNbSamples,
			(const uint8_t **)rawFrame->data, rawFrame->nb_samples);
		if (newFrame->nb_samples < 0)
		{
			qDebug() << "swr_convert failed";
			return;
		}
		//qDebug() << "newFrame nb_samples: " << newFrame->nb_samples;
		{
			unique_lock<mutex> lk(m_mtx);
			m_cvNotFull.wait(lk, [newFrame, this] { return av_audio_fifo_space(m_aFifoBuf) >= newFrame->nb_samples; });
		}
		if (av_audio_fifo_write(m_aFifoBuf, (void **)newFrame->data, newFrame->nb_samples) < newFrame->nb_samples)
		{
			qDebug() << "av_audio_fifo_write";
			return;
		}
		//m_nbSamples = newFrame->nb_samples;
		m_cvNotEmpty.notify_one();
	}
	av_frame_free(&rawFrame);
	av_frame_free(&newFrame);
	qDebug() << "sound record thread exit";
}
