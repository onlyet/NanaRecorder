#pragma once

#include "FFmpegHeader.h"

class VideoCapture;
class VideoFrameQueue;

class Recorder
{
public:
	Recorder();
	~Recorder();

	int startRecord();
	int pauseRecord();
	int stopRecord();

private:
	void startCapture();
	void stopCapture();

	void openEncoder();


	void writeFrame(AVFrame* frame, AVCodecContext* decodeCtx);


private:
	VideoCapture* m_videoCap = nullptr;
	VideoFrameQueue* m_videoFrameQueue = nullptr;

	int64_t		m_startTime;
};

