#pragma once

#include "FFmpegHeader.h"

#include <vector>

class VideoEncoder
{
public:
    int initH264(int width, int height, int fps);
    void deinit();
    int encode(AVFrame* frame, int stream_index, int64_t pts, int64_t time_base, std::vector<AVPacket*>& packets);
    AVCodecContext* codecCtx() { return m_vEncodeCtx; }

private:
    AVCodecContext* m_vEncodeCtx = nullptr;
    AVDictionary*   m_dict = nullptr;
};

