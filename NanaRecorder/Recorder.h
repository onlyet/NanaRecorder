#pragma once

#include "FFmpegHeader.h"

class VideoCapture;
class VideoFrameQueue;
class FrameItem;
class VideoCaptureInfo;

class AudioCapture;
class AudioFrameQueue;

class FileOutputer;

class Recorder
{
public:
	Recorder();
	~Recorder();

	void setRecordInfo();
	int startRecord();
	int pauseRecord();
	int stopRecord();

private:
	void startCapture();
	void stopCapture();

	void writeVideoFrameCb(AVFrame* frame, const VideoCaptureInfo& info);
	FrameItem* readVideoFrameCb();

	void initAudioBufCb(AVCodecContext* encodeCtx);
	void writeAudioFrameCb(AVFrame* frame);

private:
    VideoCapture*    m_videoCap        = nullptr;
    VideoFrameQueue* m_videoFrameQueue = nullptr;
    AudioCapture*    m_audioCap        = nullptr;
    AudioFrameQueue* m_audioFrameQueue = nullptr;
    FileOutputer*    m_outputer        = nullptr;
    int64_t          m_startTime;
};

