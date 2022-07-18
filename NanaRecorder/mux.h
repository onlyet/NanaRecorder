#pragma once
#include "FFmpegHeader.h"

#include <string>

class Mux
{
public:
    Mux();
    ~Mux();
    int init(const std::string& filename);
    void deinit();
    int writeHeader();
    int writePacket(AVPacket* packet);
    int writeTrailer();

    int addStream(AVCodecContext* encodeCtx);

private:
    AVFormatContext*    m_oFmtCtx = nullptr;
    std::string         m_filename;

    AVCodecContext*     m_vEncodeCtx = nullptr;
    AVCodecContext*     m_aEncodeCtx = nullptr;
    AVStream*           m_vStream = nullptr;
    AVStream*           m_aStream = nullptr;
    int                 m_vIndex;
    int                 m_aIndex;

};

