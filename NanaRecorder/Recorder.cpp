#include "RecordConfig.h"
#include "Recorder.h"
#include "VideoCapture.h"
#include "VideoFrameQueue.h"
#include "FileOutputer.h"

#include <chrono>

using namespace std;
using namespace std::placeholders;
using namespace std::chrono;

Recorder::Recorder()
{
	m_videoCap = new VideoCapture;
	m_videoCap->setFrameCb(bind(&Recorder::writeVideoFrameCb, this, _1, _2));
	m_videoFrameQueue = new VideoFrameQueue;
	m_outputer = new FileOutputer;
	m_outputer->setVideoFrameCb(bind(&Recorder::readVideoFrameCb, this));
}

Recorder::~Recorder()
{
	if (m_videoCap) {
		delete m_videoCap;
		m_videoCap = nullptr;
	}
	if (m_videoFrameQueue) {
		delete m_videoFrameQueue;
		m_videoFrameQueue = nullptr;
	}
	if (m_outputer) {
		delete m_outputer;
		m_outputer = nullptr;
	}
}
void Recorder::setRecordInfo()
{
	g_record.filePath = "nana.mp4";
	g_record.width = 1920;
	g_record.height = 1080;
	g_record.fps = 25;
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
	if (Running == g_record.status) return -1;

	startCapture();
	// init
	m_videoFrameQueue->initBuf(g_record.width, g_record.height, AV_PIX_FMT_YUV420P);

	m_outputer->init();

	// start
	m_startTime = duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
	m_outputer->start();

	g_record.status = Running;

	return 0;
}

int Recorder::pauseRecord()
{
	//stopCapture();
	return 0;
}

int Recorder::stopRecord()
{
	if (Stopped == g_record.status) return -1;

	stopCapture();
	m_outputer->stop();
	m_outputer->deinit();
	m_videoFrameQueue->deinit();
	g_record.status = Stopped;
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

void Recorder::writeVideoFrameCb(AVFrame* frame, const VideoCaptureInfo& info)
{
	if (Running == g_record.status) {
        int64_t now = duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
        int64_t captureTime = now - m_startTime;
        m_videoFrameQueue->writeFrame(frame, info, captureTime);
	}
}

FrameItem* Recorder::readVideoFrameCb()
{
	return m_videoFrameQueue->readFrame();
}
