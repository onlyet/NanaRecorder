#pragma once
#include <Windows.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
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

//主线程：Qt GUI线程
//父线程：RecordAudioThreadProc
//子线程：AcquireSoundThreadProc
//测试发现音频编码速度比采集速度要快，跟视频相反
//发现父线程（编码）可能会在子线程之前退出（采集）
//如果是这样的话，可以将子线程.join()放在父线程释放资源之前

class ScreenRecordImpl : public QObject
{
	Q_OBJECT
public:
	ScreenRecordImpl(QObject * parent = Q_NULLPTR);

	private slots :
	void Start();
	void Pause();
	void Stop();

private:
	//从fifobuf读取音频帧，编码，写入输出流，生成文件
	void RecordAudioThreadProc();
	//从音频输入流读取帧，写入fifobuf
	void AcquireSoundThreadProc();
	int OpenAudio();
	int OpenOutput();
	QString GetSpeakerDeviceName();
	QString GetMicrophoneDeviceName();
	AVFrame* AllocAudioFrame(AVCodecContext* c, int nbSamples);
	//取出编码器里的帧，写入输出流
	void FlushEncoder();

private:
	QString				m_filePath;

	int m_aIndex;		//输入音频流索引
	int m_aOutIndex;	//输出音频流索引
	AVFormatContext		*m_aFmtCtx;
	AVFormatContext		*m_oFmtCtx;
	AVCodecContext		*m_aDecodeCtx;
	AVCodecContext		*m_aEncodeCtx;
	SwrContext			*m_swrCtx;
	AVAudioFifo			*m_aFifoBuf;

	//AVFrame				*m_aOutFrame;
	//uint8_t				*m_aOutFrameBuf;
	int					m_aOutFrameSize;	//一个音频帧包含的样本数
	std::atomic_bool	m_stop;

	//Frame里单个通道的样本数
	//暂时把输入流所有的nb_samples当做一样的
	int					m_nbSamples;	
	std::mutex			m_mtx;
	std::condition_variable	m_cvNotEmpty;
	std::condition_variable m_cvNotFull;
};