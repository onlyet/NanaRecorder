#pragma once
#include "FFmpegHeader.h"

#include <string>
#include <mutex>

class Mux
{
public:
    Mux() {}
    ~Mux() {}
    int init(const std::string& filename);
    void deinit();
    int writeHeader();
    int writePacket(AVPacket* packet);
    int writeTrailer();

    int addStream(AVCodecContext* encodeCtx);

    int videoStreamIndex() { return m_vIndex; }
    int audioStreamIndex() { return m_aIndex; }

private:
    bool                m_isInit = false;
    std::string         m_filename;
    AVFormatContext*    m_oFmtCtx = nullptr;
    AVStream*           m_vStream = nullptr;
    AVStream*           m_aStream = nullptr;
    AVCodecContext*     m_vEncodeCtx = nullptr; // EncoderÓµÓÐ×ÊÔ´
    AVCodecContext*     m_aEncodeCtx = nullptr;
    int                 m_vIndex = -1;
    int                 m_aIndex = -1;
    std::mutex          m_WriteFrameMtx;
};

