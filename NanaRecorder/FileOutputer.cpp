#include "FileOutputer.h"
#include "VideoEncoder.h"
#include "mux.h"
#include "RecordConfig.h"
#include "VideoFrameQueue.h"

#include <QDebug>

#include <string>
#include <functional>


using namespace std;

FileOutputer::FileOutputer()
{
    m_videoEncoder = new VideoEncoder;
    m_mux = new Mux;
}

FileOutputer::~FileOutputer()
{
    if (m_videoEncoder) {
        delete m_videoEncoder;
        m_videoEncoder = nullptr;
    }
    if (m_mux) {
        delete m_mux;
        m_mux = nullptr;
    }
}

int FileOutputer::init()
{
    openEncoder();
    string filename = g_record.filePath.toStdString();
    m_mux->init(filename);
    m_mux->addStream(m_videoEncoder->codecCtx());
    m_mux->writeHeader();
    m_isInit = true;
    return 0;
}

int FileOutputer::deinit()
{
    m_isInit = false;
    m_mux->writeTrailer();
    m_mux->deinit();
    closeEncoder();
    return 0;
}

int FileOutputer::start()
{
    if (!m_isInit) return -1;
    m_isRunning = true;
    std::thread t(std::bind(&FileOutputer::outputVideoThreadProc, this));
    m_outputVideoThread.swap(t);
    return 0;
}

int FileOutputer::stop()
{
    m_isRunning = false;
    if (m_outputVideoThread.joinable()) {
        m_outputVideoThread.join();
    }
    return 0;
}

void FileOutputer::openEncoder()
{
    if (!m_videoEncoder) return;
    m_videoEncoder->initH264(g_record.width, g_record.height, g_record.fps);
}

void FileOutputer::closeEncoder()
{
    if (!m_videoEncoder) return;
    m_videoEncoder->deinit();
}

void FileOutputer::outputVideoThreadProc()
{
    while (m_isRunning) {
        encodeVideoAndMux();
    }
    qDebug() << "outputVideoThreadProc thread exit";
}

void FileOutputer::encodeVideoAndMux()
{
    if (!m_isInit || !m_isRunning || !m_videoEncoder || !m_mux) {
        return;
    }

    FrameItem* item;
    while (1) {
		item = m_videoFrameCb();
		if (!item) return;

		m_videoEncoder->encode(item->frame, m_mux->videoStreamIndex(), 0, 0, m_packets);

		for_each(m_packets.cbegin(), m_packets.cend(), [this, item](AVPacket* packet) {
			m_mux->writePacket(packet, item->captureTime);
			});

		m_packets.clear();
    }
}
