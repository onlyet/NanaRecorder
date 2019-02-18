#pragma once

#include <atomic>
#include <QObject>
#include <QString>
#include <QMutex>

#include <Windows.h>

#ifdef	__cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
};
#endif

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFifoBuffer;
struct AVAudioFifo;
struct AVFrame;
struct SwsContext;

class ScreenRecordImpl : public QObject
{
	Q_OBJECT
public:
	ScreenRecordImpl();

	int OpenVideo();
	int OpenAudio();
	int OpenOutput();

	private slots:
	void Start();

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
	AVFormatContext		*m_vFmtCtx;
	AVFormatContext		*m_aFmtCtx;
	AVFormatContext		*m_oFmtCtx;
	AVCodecContext		*m_vDecodeCtx;
	AVCodecContext		*m_aDecodeCtx;
	AVCodecContext		*m_vEncodeCtx;
	AVCodecContext		*m_aEncodeCtx;
	SwsContext			*m_swsCtx;
	AVFifoBuffer		*m_vBuf;
	AVAudioFifo			*m_aBuf;

	AVFrame				*m_vOutFrame;
	AVFrame				*m_aOutFrame;
	uint8_t				*m_vOutFrameBuf;
	uint8_t				*m_aOutFrameBuf;
	int					m_vOutFrameSize;
	int					m_aOutFrameSize;
	std::atomic_bool	m_stop;
	int					m_videoFrameSize;

	CRITICAL_SECTION	m_vSection, m_aSection;
	int					m_vFrameIndex, m_aFrameIndex;	//当前帧位置

	bool				m_started;
};