#pragma once
#include "FFmpegHeader.h"

class AudioEncoder {
public:
    int             initAAC();
    void            deinit();
    int             encode(AVFrame* frame, int stream_index, int64_t pts, int64_t time_base, std::vector<AVPacket*>& packets);
    AVCodecContext* codecCtx() { return m_vEncodeCtx; }

private:
    AVCodecContext* m_vEncodeCtx = nullptr;
    AVDictionary*   m_dict       = nullptr;
};
