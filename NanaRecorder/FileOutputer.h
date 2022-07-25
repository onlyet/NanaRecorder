#pragma once
#include "FFmpegHeader.h"

#include <thread>
#include <functional>
#include <vector>

class VideoEncoder;
class AudioEncoder;
class Mux;
class FrameItem;

class FileOutputer
{
public:
    FileOutputer();
    ~FileOutputer();
    void setVideoFrameCb(std::function<FrameItem*()> cb) { m_videoFrameCb = cb; }
    void setAudioBufCb(std::function<void(AVCodecContext*)> cb) { m_initAudioBufCb = cb; }
    int init();
    int deinit();
    int start();
    int stop();

private:
    void openEncoder();
    void closeEncoder();
    void outputVideoThreadProc();
    void encodeVideoAndMux();

private:
    std::function<FrameItem*()>          m_videoFrameCb;
    std::function<void(AVCodecContext*)> m_initAudioBufCb;
    bool                                 m_isInit       = false;
    bool                                 m_isRunning    = false;
    VideoEncoder*                        m_videoEncoder = nullptr;
    AudioEncoder*                        m_audioEncoder = nullptr;
    Mux*                                 m_mux          = nullptr;
    std::thread                          m_outputVideoThread;
    std::vector<AVPacket*>               m_packets;
};

