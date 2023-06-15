#include <chrono>
#include <sstream>

#include "ResampleFilter.h"
#include "FFmpegHeader.h"

#include <QDebug>

namespace onlyet {

ResampleFilter::ResampleFilter() {
    memset(&_ctx_in, 0, sizeof(FILTER_CTX));
    memset(&_ctx_out, 0, sizeof(FILTER_CTX));

    _filter_graph = NULL;

    _inited  = false;
    _running = false;

    _cond_notify = false;

    //_index = -1;
}

ResampleFilter::~ResampleFilter() {
    stop();
    cleanup();
}

int ResampleFilter::init(const FILTER_CTX& ctx_in, const FILTER_CTX& ctx_out) {
    int error = 0;
    int ret   = 0;

    if (_inited) return error;

    //_index = index;

    do {
        _ctx_in  = ctx_in;
        _ctx_out = ctx_out;

        _filter_graph = avfilter_graph_alloc();
        if (!_filter_graph) {
            error = -1;
            break;
        }

        char layout_name[256] = {0};
        av_get_channel_layout_string(layout_name, 256, ctx_out.nb_channel, ctx_out.channel_layout);

        std::stringstream filter_desrcss;
        filter_desrcss << "aresample=";
        filter_desrcss << ctx_out.sample_rate;
        filter_desrcss << ",aformat=sample_fmts=";
        filter_desrcss << av_get_sample_fmt_name(ctx_out.sample_fmt);
        filter_desrcss << ":channel_layouts=";
        filter_desrcss << layout_name;

        std::string filter_desrc = filter_desrcss.str();

        _ctx_in.inout  = avfilter_inout_alloc();
        _ctx_out.inout = avfilter_inout_alloc();

        char pad_args[512] = {0};

        format_pad_arg(pad_args, 512, _ctx_in);

        ret = avfilter_graph_create_filter(&_ctx_in.ctx, avfilter_get_by_name("abuffer"), "in", pad_args, NULL, _filter_graph);
        if (ret < 0) {
            error = -1;
            break;
        }

        ret = avfilter_graph_create_filter(&_ctx_out.ctx, avfilter_get_by_name("abuffersink"), "out", NULL, NULL, _filter_graph);
        if (ret < 0) {
            error = -1;
            break;
        }

        av_opt_set_bin(_ctx_out.ctx, "sample_fmts", (uint8_t*)&_ctx_out.sample_fmt, sizeof(_ctx_out.sample_fmt), AV_OPT_SEARCH_CHILDREN);
        av_opt_set_bin(_ctx_out.ctx, "channel_layouts", (uint8_t*)&_ctx_out.channel_layout, sizeof(_ctx_out.channel_layout), AV_OPT_SEARCH_CHILDREN);
        av_opt_set_bin(_ctx_out.ctx, "sample_rates", (uint8_t*)&_ctx_out.sample_rate, sizeof(_ctx_out.sample_rate), AV_OPT_SEARCH_CHILDREN);

        _ctx_in.inout->name       = av_strdup("in");
        _ctx_in.inout->filter_ctx = _ctx_in.ctx;
        _ctx_in.inout->pad_idx    = 0;
        _ctx_in.inout->next       = NULL;

        _ctx_out.inout->name       = av_strdup("out");
        _ctx_out.inout->filter_ctx = _ctx_out.ctx;
        _ctx_out.inout->pad_idx    = 0;
        _ctx_out.inout->next       = NULL;

        ret = avfilter_graph_parse_ptr(_filter_graph, filter_desrc.c_str(), &_ctx_out.inout, &_ctx_in.inout, NULL);
        if (ret < 0) {
            error = -1;
            break;
        }

        ret = avfilter_graph_config(_filter_graph, NULL);
        if (ret < 0) {
            error = -1;
            break;
        }

        _inited = true;
    } while (0);

    if (error != 0) {
        qCritical() << "filter init failed";
        cleanup();
    }

    return error;
}

int ResampleFilter::start() {
    if (!_inited)
        return -1;

    if (_running)
        return 0;

    _running = true;
    _thread  = std::thread(std::bind(&ResampleFilter::filter_loop, this));

    return 0;
}

int ResampleFilter::stop() {
    if (!_inited || !_running)
        return -1;

    _running = false;

    _cond_notify = true;
    _cond_var.notify_all();

    if (_thread.joinable())
        _thread.join();

    return 0;
}

int ResampleFilter::add_frame(AVFrame* frame) {
    std::unique_lock<std::mutex> lock(_mutex);

    int error = 0;
    int ret   = 0;

    do {
        int ret = av_buffersrc_add_frame_flags(_ctx_in.ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
        if (ret < 0) {
            error = -1;
            break;
        }

    } while (0);

    if (error != 0) {
        qCritical() << QString("av_buffersrc_add_frame_flags ret:%1").arg(ret);
    }

    _cond_notify = true;
    _cond_var.notify_all();

    return error;
}

const AVRational ResampleFilter::get_time_base() {
    return av_buffersink_get_time_base(_ctx_out.ctx);
}

void ResampleFilter::cleanup() {
    if (_filter_graph)
        avfilter_graph_free(&_filter_graph);

    memset(&_ctx_in, 0, sizeof(FILTER_CTX));
    memset(&_ctx_out, 0, sizeof(FILTER_CTX));

    _inited = false;
}

void ResampleFilter::filter_loop() {
    AVFrame* frame = av_frame_alloc();

    int ret = 0;
    while (_running) {
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_cond_notify && _running)
            _cond_var.wait_for(lock, std::chrono::milliseconds(300));

        while (_running && _cond_notify) {
            ret = av_buffersink_get_frame(_ctx_out.ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
                ;
            }

            if (ret < 0) {
                qCritical() << "av_buffersink_get_frame failed:" << ret;
                //if (_on_filter_error) _on_filter_error(ret, _index);
                break;
            }

            if (_on_filter_data)
                _on_filter_data(frame, 0);

            av_frame_unref(frame);
        }

        _cond_notify = false;
    }

    av_frame_free(&frame);
}

}  // namespace onlyet