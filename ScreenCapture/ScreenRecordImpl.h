#pragma once


#include <Windows.h>
#include <atomic>
#include <QObject>
#include <QString>
#include <QMutex>
#include <condition_variable>

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
private:
	enum RecordState {
		NotStarted,
		Started,
		Paused,
		Stopped,
		Unknown,
	};
public:
	ScreenRecordImpl(QObject * parent = Q_NULLPTR);
	void Init(const QVariantMap& map);

	private slots :
	void Start();
	void Pause();
	void Stop();

private:
	//从fifobuf读取音视频帧，写入输出流，复用，生成文件
	void MuxThreadProc();
	//从视频输入流读取帧，写入fifobuf
	void ScreenRecordThreadProc();
	//从音频输入流读取帧，写入fifobuf
	void SoundRecordThreadProc();
	int OpenVideo();
	int OpenAudio();
	int OpenOutput();
	QString GetSpeakerDeviceName();
	//获取麦克风设备名称
	QString GetMicrophoneDeviceName();
	AVFrame* AllocAudioFrame(AVCodecContext* c, int nbSamples);
	void InitVideoBuffer();
	void InitAudioBuffer();
	void FlushVideoDecoder();
	void FlushAudioDecoder();
	//void FlushVideoEncoder();
	//void FlushAudioEncoder();
	void FlushEncoders();
	void Release();

private:
	QString				m_filePath;
	int					m_width;
	int					m_height;
	int					m_fps;
	int					m_audioBitrate;

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

	AVFrame				*m_vOutFrame;
	uint8_t				*m_vOutFrameBuf;
	int					m_vOutFrameSize;

	int					m_nbSamples;
	RecordState			m_state;
	std::condition_variable m_cvNotPause;	//当点击暂停的时候，两个采集线程挂起
	std::mutex				m_mtxPause;
	std::condition_variable m_cvVBufNotFull;
	std::condition_variable m_cvVBufNotEmpty;
	std::mutex				m_mtxVBuf;
	std::condition_variable m_cvABufNotFull;
	std::condition_variable m_cvABufNotEmpty;
	std::mutex				m_mtxABuf;
	int64_t					m_vCurPts;
	int64_t					m_aCurPts;
};