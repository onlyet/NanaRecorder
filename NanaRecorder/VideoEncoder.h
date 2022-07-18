#pragma once

#include "FFmpegHeader.h"


class VideoEncoder
{
public:
    int initH264(int width, int height, int fps);
    int encode();

private:
    AVCodecContext* m_vEncodeCtx = nullptr;

};

