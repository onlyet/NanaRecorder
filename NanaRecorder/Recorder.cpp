#include "Recorder.h"
#include "VideoCapture.h"
#include "VideoFrameQueue.h"
#include "RecordConfig.h"

#include <chrono>

using namespace std;
using namespace std::placeholders;
using namespace std::chrono;

Recorder::Recorder()
{
	m_videoCap = new VideoCapture;
	m_videoFrameQueue = new VideoFrameQueue;
	m_videoCap->setFrameCb(bind(&Recorder::writeFrame, this, _1, _2));
}

Recorder::~Recorder()
{
	if (m_videoCap) {
		delete m_videoCap;
		m_videoCap = nullptr;
	}
}
/**
 * 主线程：
 * 初始化FIFO buf
 * 打开编码器
 * 初始化复用器
 * 从格式上下文创建流stream，从编码器上下文拷贝参数到流
 * 
 * 采集线程：采集，缩放，写入FIFO
 * 
 * 编码复用线程：
 * 从FIFO读一帧frame
 * 编码成packet
 * 将packet写入文件
 * 
 * @return 
*/
int Recorder::startRecord()
{
	startCapture();

	openEncoder();

	m_startTime = duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();

	m_videoFrameQueue->initBuf(g_record.width, g_record.height, AV_PIX_FMT_YUV420P);

	return 0;
}

int Recorder::pauseRecord()
{
	stopCapture();
	return 0;
}

int Recorder::stopRecord()
{
	return 0;
}

void Recorder::startCapture()
{
	m_videoCap->startCapture();
}

void Recorder::stopCapture()
{
	m_videoCap->stopCapture();
}

void Recorder::openEncoder()
{
}

void Recorder::writeFrame(AVFrame* frame, AVCodecContext* decodeCtx)
{
	int64_t now = duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
	int64_t captureTime = now - m_startTime;
	m_videoFrameQueue->writeFrame(frame, decodeCtx, captureTime);
}
