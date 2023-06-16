#ifndef ONLYET_FILTER_H
#define ONLYET_FILTER_H

#include <functional>


#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>

#ifdef __cplusplus
};
#endif

class AVFilterContext;
class AVFilterInOut;
class AVFrame;

namespace onlyet {

typedef struct {
    AVFilterContext* ctx;
    AVFilterInOut*   inout;

    AVRational     time_base;
    int            sample_rate;
    AVSampleFormat sample_fmt;
    int            nb_channel;
    int64_t        channel_layout;
} FILTER_CTX;

/**
	* filter data callback
	* @param frame pointer to a AVFrame
	* @param index resource index ,default is -1
	*/
typedef std::function<void(AVFrame* frame, int index)> FilterFrameCb;

void format_pad_arg(char* arg, int size, const FILTER_CTX& ctx);

}  // namespace onlyet

#endif  // !ONLYET_FILTER_H
