#pragma once

#include "FFmpegHeader.h"

class VideoCapture;
class VideoFrameQueue;
class FileOutputer;
class FrameItem;
class VideoCaptureInfo;

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

private:
	VideoCapture* m_videoCap = nullptr;
	VideoFrameQueue* m_videoFrameQueue = nullptr;
	FileOutputer* m_outputer = nullptr;
	int64_t		m_startTime;
};

