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
    int writePacket();
    int writeTrailer();

    int addStream();

private:
    AVFormatContext*    m_oFmtCtx = nullptr;
    std::string         m_filename;
};

