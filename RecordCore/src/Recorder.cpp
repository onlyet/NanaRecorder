#include "FFmpegHeader.h"
#include <Recorder.h>
#include "RecordConfig.h"
#include "VideoCapture.h"
#include "VideoFrameQueue.h"
#include "AudioCapture.h"
#include "AudioFrameQueue.h"
#include "FileOutputer.h"
#include "FFmpegHelper.h"
#include <AmixFilter.h>

#include <util.h>
#include <timer.h>

#include <QDateTime>
#include <QDebug>
#include <QApplication>

using namespace std;
using namespace std::placeholders;
using namespace std::chrono;

RECORDAPI std::unique_ptr<IRecorder> createRecorder(const QVariantMap& recordInfo) {
    return make_unique<Recorder>(recordInfo);
}
//RECORDAPI IRecorder* freeRecorder() {
//
//}

Recorder::Recorder(const QVariantMap& recordInfo) {

    m_pauseStopwatch = make_unique<Timer<std::chrono::system_clock>>();

	setRecordInfo(recordInfo);

	m_videoCap = new VideoCapture;
	m_videoCap->setFrameCb(bind(&Recorder::writeVideoFrameCb, this, _1, _2));
    m_videoFrameQueue = new VideoFrameQueue;

    if (g_record.enableAudio) {
        m_audioSrcNum = 2;
        if (m_audioSrcNum > 1) {
            //m_amixFilter = new AmixFilter;
        }

        m_speakerCap = new AudioCapture;
        m_speakerCap->setFrameCb(bind(&Recorder::addFrameToAmixFilter, this, _1, _2), 1);

        m_microphoneCap = new AudioCapture;
        m_microphoneCap->setFrameCb(bind(&Recorder::addFrameToAmixFilter, this, _1, _2), 2);

        m_audioFrameQueue = new AudioFrameQueue;
    }

	m_outputer = new FileOutputer;
	m_outputer->setVideoFrameCb(bind(&Recorder::readVideoFrameCb, this));
    if (g_record.enableAudio) {
        m_outputer->setAudioBufCb(bind(&Recorder::initAudioBufCb, this, _1));
        m_outputer->setAudioFrameCb(bind(&Recorder::readAudioFrameCb, this));
        m_outputer->setPauseDurationCb(bind(&Recorder::getPauseDurationCb, this));
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
    if (m_speakerCap) {
        delete m_speakerCap;
        m_speakerCap = nullptr;
    }
    if (m_microphoneCap) {
        delete m_microphoneCap;
        m_microphoneCap = nullptr;
    }
    if (m_audioFrameQueue) {
        delete m_audioFrameQueue;
        m_audioFrameQueue = nullptr;
    }
	if (m_outputer) {
		delete m_outputer;
		m_outputer = nullptr;
	}
}

void Recorder::setRecordInfo(const QVariantMap& recordInfo) {
    g_record.filePath         = recordInfo["recordPath"].toString();
    g_record.inWidth          = util::screenWidth();
    g_record.inHeight         = util::screenHeight();
    g_record.outWidth         = recordInfo["outWidth"].toInt();
    g_record.outHeight        = recordInfo["outHeight"].toInt();
    g_record.fps              = recordInfo["fps"].toInt();
    g_record.enableAudio      = recordInfo["enableAudio"].toBool();
    g_record.audioDeviceIndex = recordInfo["audioDeviceIndex"].toInt();
    g_record.channel          = recordInfo["channel"].toInt();
    g_record.sampleRate       = recordInfo["sampleRate"].toInt();

    qInfo() << QString("Record info filePath:%1,inWidth:%2,inHeight:%3,outWidth:%4,outHeight:%5,\
                        fps:%6,enableAudio:%7,audioDeviceIndex:%8,channel:%9,sampleRate:%10")
                   .arg(g_record.filePath)
                   .arg(g_record.inWidth)
                   .arg(g_record.inHeight)
                   .arg(g_record.outWidth)
                   .arg(g_record.outHeight)
                   .arg(g_record.fps)
                   .arg(g_record.enableAudio)
                   .arg(g_record.audioDeviceIndex)
                   .arg(g_record.channel)
                   .arg(g_record.sampleRate);
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
    m_videoFrameQueue->initBuf(g_record.outWidth, g_record.outHeight, AV_PIX_FMT_YUV420P);

    int ret = m_amixFilter->init(
        {nullptr, nullptr,
         m_speakerCap->timebase(),
         m_speakerCap->sampleRate(),
         m_speakerCap->sampleFormat(),
         m_speakerCap->channel(),
         m_speakerCap->channelLayout()},
        {nullptr, nullptr,
         m_microphoneCap->timebase(),
         m_microphoneCap->sampleRate(),
         m_microphoneCap->sampleFormat(),
         m_microphoneCap->channel(),
         m_microphoneCap->channelLayout()},
        {nullptr, nullptr, {1, AV_TIME_BASE}, g_record.sampleRate, AV_SAMPLE_FMT_FLTP, g_record.channel, av_get_default_channel_layout(g_record.channel)});
    if (ret != 0) return -1;

    m_amixFilter->registe_cb(bind(static_cast<void (Recorder::*)(AVFrame*)>(&Recorder::writeAudioFrameCb), this, _1));

    m_outputer->init();

	// start
    m_startTime = duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
    qInfo() << "start time:" << QDateTime::fromMSecsSinceEpoch(m_startTime / 1000).toString("yyyy-MM-dd hh:mm:ss.zzz");
    m_outputer->start(m_startTime);

	g_record.status = Running;

	return 0;
}

int Recorder::pauseRecord()
{
    if (Running != g_record.status) return -1;
    g_record.status = Paused;
    m_pauseStopwatch->start();
	return 0;
}

int Recorder::resumeRecord() {
    if (Paused != g_record.status) return -1;
    g_record.status = Running;
    g_record.cvNotPause.notify_all();
    m_pauseDuration += m_pauseStopwatch->delta<Timer<>::us>();
    return 0;
}

int Recorder::stopRecord()
{
    RecordStatus oldStatus = g_record.status;
    if (Stopped == oldStatus) return -1;
    g_record.status = Stopped;

    if (Paused == oldStatus) {
        g_record.cvNotPause.notify_all();
    }

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
        int ret = m_speakerCap->startCapture(AudioCaptureDevice::Speaker);
        // 找不到音频或打开失败
        if (-1 == ret) {
            g_record.enableAudio = false;
        }
        ret = m_microphoneCap->startCapture(AudioCaptureDevice::Microphone);
        // 找不到音频或打开失败
        if (-1 == ret) {
            g_record.enableAudio = false;
        }
    }
}

void Recorder::stopCapture()
{
	m_videoCap->stopCapture();
    if (g_record.enableAudio) {
        m_speakerCap->stopCapture();
        m_microphoneCap->stopCapture();
    }
}

void Recorder::writeVideoFrameCb(AVFrame* frame, const VideoCaptureInfo& info)
{
	if (Running == g_record.status) {
        int64_t now         = duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
        int64_t captureTime = now - m_startTime - m_pauseDuration; // pts = 当前时间戳 - 开始时间戳 - 暂停总时间
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

void Recorder::addFrameToAmixFilter(AVFrame* frame, int filterCtxIndex) {
    m_amixFilter->add_frame(frame, filterCtxIndex);
}

void Recorder::writeAudioFrameCb(AVFrame* frame) {
    if (Running == g_record.status) {
        if (m_audioFrameQueue) {
            m_audioFrameQueue->writeFrame(frame/*, info*/);
        }
    }
}
