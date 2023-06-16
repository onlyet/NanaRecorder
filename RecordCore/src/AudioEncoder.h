#ifndef ONLYET_AUDIOENCODER_H
#define ONLYET_AUDIOENCODER_H



#include <vector>

class AVFrame;
class AVPacket;
class AVCodecContext;
class AVDictionary;

namespace onlyet {

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

}  // namespace onlyet

#endif  // !ONLYET_AUDIOENCODER_H
