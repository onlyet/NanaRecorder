#pragma once


#include <Windows.h>
#include <atomic>
#include <QObject>
#include <QString>
#include <QMutex>

#ifdef	__cplusplus
extern "C"
{
#endif
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFifoBuffer;
struct AVAudioFifo;
struct AVFrame;
struct SwsContext;
struct SwrContext;
#ifdef __cplusplus
};
#endif

class ScreenRecordImpl : public QObject
{
	Q_OBJECT
public:
	ScreenRecordImpl(QObject * parent = Q_NULLPTR);

	int OpenVideo();
	int OpenAudio();
	int OpenOutput();

	//signals:
	private slots :
	void Start();
	void Finish();

private:
	QString GetSpeakerDeviceName();
	QString GetMicrophoneDeviceName();
	AVFrame* AllocAudioFrame(AVCodecContext* c, int nbSamples);

private:
	//从fifobuf读取音视频帧，写入输出流，复用，生成文件
	void MuxThreadProc();
	//从视频输入流读取帧，写入fifobuf
	void ScreenRecordThreadProc();
	//从音频输入流读取帧，写入fifobuf
	void SoundRecordThreadProc();

private:

	QString				m_filePath;
	int					m_width;
	int					m_height;
	int					m_fps;

	int m_vIndex;		//输入视频流索引
	int m_aIndex;		//输入音频流索引
	int m_vOutIndex;	//输出视频流索引
	int m_aOutIndex;	//输出音频流索引
	AVFormatContext		*m_vFmtCtx;
	AVFormatContext		*m_aFmtCtx;
	AVFormatContext		*m_oFmtCtx;
	AVCodecContext		*m_vDecodeCtx;
	AVCodecContext		*m_aDecodeCtx;
	AVCodecContext		*m_vEncodeCtx;
	AVCodecContext		*m_aEncodeCtx;
	SwsContext			*m_swsCtx;
	SwrContext			*m_swrCtx;
	AVFifoBuffer		*m_vFifoBuf;
	AVAudioFifo			*m_aFifoBuf;
	//int					m_vInFrameSize;	//视频输入帧大小

	AVFrame				*m_vOutFrame;
	//AVFrame				*m_aOutFrame;
	uint8_t				*m_vOutFrameBuf;
	//uint8_t				*m_aOutFrameBuf;
	int					m_vOutFrameSize;
	int					m_aOutFrameSize;	//一个音频帧包含的样本数
	std::atomic_bool	m_stop;
	//int					m_videoFrameSize;

	CRITICAL_SECTION	m_vSection, m_aSection;
	int					m_vFrameIndex, m_aFrameIndex;	//当前帧位置

	bool				m_started;
};