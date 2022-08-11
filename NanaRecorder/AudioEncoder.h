#pragma once
#include "FFmpegHeader.h"

#include <vector>

class AudioEncoder {
public:
    int             initAAC();
    void            deinit();
    int             encode(AVFrame* frame, int stream_index, int64_t pts, int64_t time_base, std::vector<AVPacket*>& packets);
    AVCodecContext* codecCtx() { return m_aEncodeCtx; }

private:
    AVCodecContext* m_aEncodeCtx = nullptr;
    AVDictionary*   m_dict       = nullptr;
    int             m_channel;
};
