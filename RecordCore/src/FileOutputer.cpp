#include "FFmpegHeader.h"
#include "FileOutputer.h"
#include "RecordConfig.h"
#include "VideoEncoder.h"
#include "VideoFrameQueue.h"
#include "AudioEncoder.h"
#include "mux.h"

#include <QDebug>

#include <string>
#include <chrono>

using namespace std;
using namespace std::chrono;

namespace onlyet {

FileOutputer::FileOutputer() {
    m_enableAudio  = g_record.enableAudio;
    m_videoEncoder = new VideoEncoder;
    if (m_enableAudio) {
        m_audioEncoder = new AudioEncoder;
    }
    m_mux = new Mux;
}

FileOutputer::~FileOutputer() {
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

int FileOutputer::init() {
    openEncoder();

    string filename = g_record.filePath.toStdString();
    m_mux->init(filename);
    m_mux->addStream(m_videoEncoder->codecCtx());
    if (m_audioEncoder) {
        m_mux->addStream(m_audioEncoder->codecCtx());
    }
    m_mux->writeHeader();
    m_isInit = true;
    return 0;
}

int FileOutputer::deinit() {
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

    m_outputVideoThread = thread(bind(&FileOutputer::outputVideoThreadProc, this));

    if (m_enableAudio) {
        m_outputAudioThread = thread(bind(&FileOutputer::outputAudioThreadProc, this));
    }
    return 0;
}

int FileOutputer::stop() {
    m_isRunning = false;
    if (m_outputVideoThread.joinable()) {
        m_outputVideoThread.join();
    }
    if (m_enableAudio && m_outputAudioThread.joinable()) {
        m_outputAudioThread.join();
    }
    return 0;
}

void FileOutputer::openEncoder() {
    if (!m_videoEncoder) return;
    m_videoEncoder->initH264(g_record.outWidth, g_record.outHeight, g_record.fps);

    if (!m_audioEncoder) return;
    m_audioEncoder->initAAC();
    if (m_initAudioBufCb) {
        m_initAudioBufCb(m_audioEncoder->codecCtx());
    }
}

void FileOutputer::closeEncoder() {
    if (!m_videoEncoder) return;
    m_videoEncoder->deinit();

    if (m_audioEncoder) {
        m_audioEncoder->deinit();
    }
}

void FileOutputer::outputVideoThreadProc() {
    while (m_isRunning) {
        encodeVideoAndMux();
    }
    qInfo() << "outputVideoThreadProc thread exit";
}

void FileOutputer::encodeVideoAndMux() {
    if (!m_isInit || !m_isRunning || !m_videoEncoder || !m_mux) {
        return;
    }

    FrameItem* item;
    while (1) {
        item = m_videoFrameCb();
        if (!item) {
            //qDebug() << "m_videoFrameCb is null";
            return;
        }

        m_captureTimeQueue.push(item->captureTime);

        // 8次EAGAIN后avcodec_receive_packet才成功
        m_videoEncoder->encode(item->frame, m_mux->videoStreamIndex(), 0, 0, m_videoPackets);

        if (m_videoPackets.empty()) return;

        for_each(m_videoPackets.cbegin(), m_videoPackets.cend(), [this](AVPacket* packet) {
            if (!m_captureTimeQueue.empty()) {
                m_mux->writePacket(packet, m_captureTimeQueue.front());
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
    qInfo() << "outputAudioThreadProc thread exit";
}

void FileOutputer::encodeAudioAndMux() {
    if (!m_isInit || !m_isRunning || !m_audioEncoder || !m_mux) {
        return;
    }

    if (!m_audioFrameCb || !m_pauseCb) {
        return;
    }

    AVFrame* frame;
    while (1) {
        frame = m_audioFrameCb();
        if (!frame) return;

        m_audioEncoder->encode(frame, m_mux->audioStreamIndex(), 0, 0, m_audioPackets);

        if (m_audioPackets.empty()) return;

        int64_t now           = duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
        int64_t pauseDuration = m_pauseCb();
        int64_t captureTime   = now - m_startTime - pauseDuration;  // pts = 当前时间戳 - 开始时间戳 - 暂停总时间
        //qDebug() << QString("Audio captureTime:%1,pauseDuration:%2").arg(captureTime).arg(pauseDuration);
        for_each(m_audioPackets.cbegin(), m_audioPackets.cend(), [this, &captureTime](AVPacket* packet) {
            m_mux->writePacket(packet, captureTime);
        });

        m_audioPackets.clear();
    }
}

}  // namespace onlyet