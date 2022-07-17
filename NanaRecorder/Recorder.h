#pragma once

class VideoCapture;

class Recorder
{
public:
	Recorder();

	int startRecord();
	int pauseRecord();
	int stopRecord();

private:
	void startCapture();
	void stopCapture();

private:
	VideoCapture* m_videoCap = nullptr;
};

