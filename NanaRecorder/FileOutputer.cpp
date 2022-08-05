#include "FileOutputer.h"
#include "RecordConfig.h"
#include "VideoEncoder.h"
#include "VideoFrameQueue.h"
#include "AudioEncoder.h"
#include "mux.h"

#include <QDebug>

#include <string>
#include <functional>
#include <chrono>

#include <Windows.h>

using namespace std;
using namespace std::chrono;


FileOutputer::FileOutputer()
{
    m_videoEncoder = new VideoEncoder;
    m_audioEncoder = new AudioEncoder;
    m_mux = new Mux;
}

FileOutputer::~FileOutputer()
{
    if (m_videoEncoder) {
        delete m_videoEncoder;
        m_videoEncoder = nullptr;
    }
    if (m_audioEncoder) {
        delete m_audioEncoder;
        m_audioEncoder = nullptr;
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
    m_mux->addStream(m_audioEncoder->codecCtx());
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

int FileOutputer::start(int64_t startTime) {
    if (!m_isInit) return -1;
    m_isRunning = true;
    m_startTime = startTime;
    thread vt(bind(&FileOutputer::outputVideoThreadProc, this));
    m_outputVideoThread.swap(vt);
    //SetThreadPriority(m_outputVideoThread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL);

   thread at(bind(&FileOutputer::outputAudioThreadProc, this));
    m_outputAudioThread.swap(at);
   //SetThreadPriority(m_outputAudioThread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL);
   return 0;
}

int FileOutputer::stop()
{
    m_isRunning = false;
    if (m_outputVideoThread.joinable()) {
        m_outputVideoThread.join();
    }
    if (m_outputAudioThread.joinable()) {
        m_outputAudioThread.join();
    }
    return 0;
}

void FileOutputer::openEncoder() {
    if (!m_videoEncoder) return;
    m_videoEncoder->initH264(g_record.width, g_record.height, g_record.fps);

    if (!m_audioEncoder) return;
    m_audioEncoder->initAAC();
    m_initAudioBufCb(m_audioEncoder->codecCtx());
}

void FileOutputer::closeEncoder()
{
    if (!m_videoEncoder) return;
    m_videoEncoder->deinit();

    m_audioEncoder->deinit();
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
        if (!item) {
            qDebug() << "m_videoFrameCb is null";
            return;
        }

        m_captureTimeQueue.push(item->captureTime);

        // 8��EAGAIN��avcodec_receive_packet�ųɹ�
		m_videoEncoder->encode(item->frame, m_mux->videoStreamIndex(), 0, 0, m_videoPackets);

        if (m_videoPackets.empty()) return;

		for_each(m_videoPackets.cbegin(), m_videoPackets.cend(), [this/*, item*/](AVPacket* packet) {
            if (!m_captureTimeQueue.empty()) {
                m_mux->writePacket(packet, /*item->captureTime*/ m_captureTimeQueue.front());
                m_captureTimeQueue.pop();
            }
        });

		m_videoPackets.clear();
    }
}

void FileOutputer::outputAudioThreadProc() {
    while (m_isRunning) {
        encodeAudioAndMux();
    }
    qDebug() << "outputAudioThreadProc thread exit";
}

void FileOutputer::encodeAudioAndMux() {
    if (!m_isInit || !m_isRunning || !m_audioEncoder || !m_mux) {
        return;
    }

    if (!m_audioFrameCb) {
        return;
    }

    AVFrame* frame;
    while (1) {
        frame = m_audioFrameCb();
        if (!frame) return;

        m_audioEncoder->encode(frame, m_mux->audioStreamIndex(), 0, 0, m_audioPackets);

        if (m_audioPackets.empty()) return;

        int64_t now         = duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        int64_t captureTime = now - m_startTime;

        for_each(m_audioPackets.cbegin(), m_audioPackets.cend(), [this, &captureTime](AVPacket* packet) {
            m_mux->writePacket(packet, captureTime);
        });

        m_audioPackets.clear();
    }
}