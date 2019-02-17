#pragma once

#include <QString>

#ifdef	__cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
};
#endif

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFifoBuffer;

class ScreenRecordImpl
{
public:
	ScreenRecordImpl();

	int OpenVideo();
	int OpenAudio();
	int OpenOutput();

private:
	void MuxThreadProc();

private:

	QString m_filePath;
	int m_width;
	int m_height;
	int m_fps;

	int m_vIndex;
	int m_aIndex;
	AVFormatContext	*m_vFmtCtx;
	AVFormatContext	*m_aFmtCtx;
	AVFormatContext *m_oFmtCtx;
	AVCodecContext	*m_vCodecCtx;
	AVCodecContext  *m_aCodecCtx;
	AVCodecContext  *m_vEncodeCtx;
	AVCodecContext  *m_aEncodeCtx;
	AVCodec			*m_vCodec;
	AVCodec			*m_aCodec;
	SwsContext		*m_swsCtx;
	AVFifoBuffer	*m_vBuf;
	AVFifoBuffer	*m_aBuf;

	QMutex			m_vMtx;
	QMutex			m_aMtx;
};