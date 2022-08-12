#pragma once
#include "FFmpegHeader.h"

#include <thread>
#include <functional>
#include <vector>
#include <queue>

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
    void setAudioFrameCb(std::function<AVFrame*()> cb) { m_audioFrameCb = cb; }
    void setPauseDurationCb(std::function<int64_t()> cb) { m_pauseCb = cb; }
    int  init();
    int  deinit();
    int  start(int64_t startTime);
    int  stop();

private:
    void openEncoder();
    void closeEncoder();
    void outputVideoThreadProc();
    void encodeVideoAndMux();
    void outputAudioThreadProc();
    void encodeAudioAndMux();

private:
    bool                                 m_enableAudio;
    std::function<FrameItem*()>          m_videoFrameCb;
    std::function<void(AVCodecContext*)> m_initAudioBufCb;
    std::function<AVFrame*()>            m_audioFrameCb;
    bool                                 m_isInit       = false;
    bool                                 m_isRunning    = false;
    VideoEncoder*                        m_videoEncoder = nullptr;
    AudioEncoder*                        m_audioEncoder = nullptr;
    Mux*                                 m_mux          = nullptr;
    std::thread                          m_outputVideoThread;
    std::vector<AVPacket*>               m_videoPackets;
    std::thread                          m_outputAudioThread;
    std::vector<AVPacket*>               m_audioPackets;
    int64_t                              m_startTime;
    std::queue<int64_t>                  m_captureTimeQueue;  // ������Ƶ�����ӳ�֡�Ĳ���ʱ���
    std::function<int64_t()>             m_pauseCb;           // ��ȡ��ͣ����ʱ��
};

