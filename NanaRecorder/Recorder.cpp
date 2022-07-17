#include "Recorder.h"
#include "VideoCapture.h"

Recorder::Recorder()
{
	m_videoCap = new VideoCapture;
}

int Recorder::startRecord()
{
	startCapture();

	return 0;
}

int Recorder::pauseRecord()
{
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
}
