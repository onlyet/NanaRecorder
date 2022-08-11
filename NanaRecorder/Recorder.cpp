#include "RecordConfig.h"
#include "Recorder.h"
#include "VideoCapture.h"
#include "VideoFrameQueue.h"
#include "AudioCapture.h"
#include "AudioFrameQueue.h"
#include "FileOutputer.h"
#include "FFmpegHelper.h"

#include <chrono>

#include <QDateTime>
#include <QDebug>

using namespace std;
using namespace std::placeholders;
using namespace std::chrono;

Recorder::Recorder(const QVariantMap& recordInfo) {

	setRecordInfo(recordInfo);

	m_videoCap = new VideoCapture;
	m_videoCap->setFrameCb(bind(&Recorder::writeVideoFrameCb, this, _1, _2));
    m_videoFrameQueue = new VideoFrameQueue;

    if (g_record.enableAudio) {
        m_audioCap = new AudioCapture;
        m_audioCap->setFrameCb(bind(&Recorder::writeAudioFrameCb, this, _1, _2));
        m_audioFrameQueue = new AudioFrameQueue;
    }

	m_outputer = new FileOutputer;
	m_outputer->setVideoFrameCb(bind(&Recorder::readVideoFrameCb, this));
    if (g_record.enableAudio) {
        m_outputer->setAudioBufCb(bind(&Recorder::initAudioBufCb, this, _1));
        m_outputer->setAudioFrameCb(bind(&Recorder::readAudioFrameCb, this));
    }
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

void Recorder::setRecordInfo(const QVariantMap& recordInfo) {
	g_record.filePath = "nana.mp4";
	g_record.width = 1920;
	g_record.height = 1080;
	g_record.fps = 25;

    g_record.enableAudio      = recordInfo["enableAudio"].toBool();
    g_record.audioDeviceIndex = recordInfo["audioDeviceIndex"].toInt();
    g_record.channel          = recordInfo["channel"].toInt();
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

	FFmpegHelper::registerAll();

	startCapture();
	// init
	m_videoFrameQueue->initBuf(g_record.width, g_record.height, AV_PIX_FMT_YUV420P);

	m_outputer->init();

	// start
    m_startTime = duration_cast<chrono::/*milliseconds*/ microseconds>(chrono::system_clock::now().time_since_epoch()).count();
    qDebug() << "start time:" << QDateTime::fromMSecsSinceEpoch(m_startTime).toString("yyyy-MM-dd hh:mm:ss.zzz");
    m_outputer->start(m_startTime);

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
    if (g_record.enableAudio) {
        m_audioFrameQueue->deinit();
    }
	g_record.status = Stopped;
	return 0;
}

void Recorder::startCapture()
{
	m_videoCap->startCapture();
    if (g_record.enableAudio) {
        m_audioCap->startCapture();
    }
}

void Recorder::stopCapture()
{
	m_videoCap->stopCapture();
    if (g_record.enableAudio) {
        m_audioCap->stopCapture();
    }
}

void Recorder::writeVideoFrameCb(AVFrame* frame, const VideoCaptureInfo& info)
{
	if (Running == g_record.status) {
        int64_t now         = duration_cast<chrono::/*milliseconds*/ microseconds>(chrono::system_clock::now().time_since_epoch()).count();
        int64_t captureTime = now - m_startTime;
        m_videoFrameQueue->writeFrame(frame, info, captureTime);
	}
}

FrameItem* Recorder::readVideoFrameCb()
{
	return m_videoFrameQueue->readFrame();
}

void Recorder::initAudioBufCb(AVCodecContext* encodeCtx) {
    if (m_audioFrameQueue) {
        m_audioFrameQueue->initBuf(encodeCtx);
    }
}

void Recorder::writeAudioFrameCb(AVFrame* frame, const AudioCaptureInfo& info) {
    if (Running == g_record.status) {
        if (m_audioFrameQueue) {
            m_audioFrameQueue->writeFrame(frame, info);
        }
    }
}

AVFrame* Recorder::readAudioFrameCb() {
    if (m_audioFrameQueue) {
        return m_audioFrameQueue->readFrame();
    }
    return nullptr;
}
