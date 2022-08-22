#pragma once

//#include <FFmpegHeader.h>
//#include <timer.h>

#ifdef RECORDER_EXPORT
#define RECORDAPI __declspec(dllexport)
#else
#define RECORDAPI __declspec(dllimport)
#endif  // RECORDER_EXPORT


#include <QVariant>

#include <chrono>

class AVFrame;
class AVCodecContext;

class VideoCapture;
class VideoFrameQueue;
class FrameItem;
class VideoCaptureInfo;
class AudioCapture;
class AudioFrameQueue;
class AudioCaptureInfo;
class FileOutputer;

template <typename clock_type>
class Timer;

class RECORDAPI Recorder {
public:
    Recorder(const QVariantMap& recordInfo);
	~Recorder();

	void setRecordInfo(const QVariantMap& recordInfo);
    int  startRecord();
    int  pauseRecord();
    int  resumeRecord();
    int  stopRecord();

private:
	void startCapture();
	void stopCapture();

	void writeVideoFrameCb(AVFrame* frame, const VideoCaptureInfo& info);
	FrameItem* readVideoFrameCb();

	void initAudioBufCb(AVCodecContext* encodeCtx);
    void writeAudioFrameCb(AVFrame* frame, const AudioCaptureInfo& info);
    AVFrame* readAudioFrameCb();

    int64_t getPauseDurationCb() { return m_pauseDuration; }

private:
    VideoCapture*                     m_videoCap        = nullptr;
    VideoFrameQueue*                  m_videoFrameQueue = nullptr;
    AudioCapture*                     m_audioCap        = nullptr;
    AudioFrameQueue*                  m_audioFrameQueue = nullptr;
    FileOutputer*                     m_outputer        = nullptr;
    int64_t                           m_startTime       = -1;       // ¼�ƿ�ʼʱ�����΢�룩
    int64_t                           m_pauseDuration   = 0;        // ��ͣ����ʱ��
    Timer<std::chrono::system_clock>* m_pauseStopwatch  = nullptr;  // ��ͣ���
};

