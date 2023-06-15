#include <chrono>

#include "AmixFilter.h"
#include "FFmpegHeader.h"

#include <QDebug>

namespace onlyet {

AmixFilter::AmixFilter() {
    memset(&_ctx_in_0, 0, sizeof(FILTER_CTX));
    memset(&_ctx_in_1, 0, sizeof(FILTER_CTX));
    memset(&_ctx_out, 0, sizeof(FILTER_CTX));

    _filter_graph = NULL;
    _inited       = false;
    _running      = false;
    _cond_notify  = false;
}

AmixFilter::~AmixFilter() {
    stop();
    cleanup();
}

int AmixFilter::init(const FILTER_CTX& ctx_in0, const FILTER_CTX& ctx_in1, const FILTER_CTX& ctx_out) {
    int error = 0;
    int ret   = 0;

    if (_inited) return error;

    do {
        _ctx_in_0 = ctx_in0;
        _ctx_in_1 = ctx_in1;
        _ctx_out  = ctx_out;

        _filter_graph = avfilter_graph_alloc();
        if (!_filter_graph) {
            error = -1;
            break;
        }

        const std::string filter_desrc = "[in0][in1]amix=inputs=2:duration=first:dropout_transition=0[out]";

        _ctx_in_0.inout = avfilter_inout_alloc();
        _ctx_in_1.inout = avfilter_inout_alloc();
        _ctx_out.inout  = avfilter_inout_alloc();

        char pad_args0[512] = {0}, pad_args1[512] = {0};

        format_pad_arg(pad_args0, 512, _ctx_in_0);
        format_pad_arg(pad_args1, 512, _ctx_in_1);

        ret = avfilter_graph_create_filter(&_ctx_in_0.ctx, avfilter_get_by_name("abuffer"), "in0", pad_args0, NULL, _filter_graph);
        if (ret < 0) {
            error = -1;
            break;
        }

        ret = avfilter_graph_create_filter(&_ctx_in_1.ctx, avfilter_get_by_name("abuffer"), "in1", pad_args1, NULL, _filter_graph);
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

        _ctx_in_0.inout->name       = av_strdup("in0");
        _ctx_in_0.inout->filter_ctx = _ctx_in_0.ctx;
        _ctx_in_0.inout->pad_idx    = 0;
        _ctx_in_0.inout->next       = _ctx_in_1.inout;

        _ctx_in_1.inout->name       = av_strdup("in1");
        _ctx_in_1.inout->filter_ctx = _ctx_in_1.ctx;
        _ctx_in_1.inout->pad_idx    = 0;
        _ctx_in_1.inout->next       = NULL;

        _ctx_out.inout->name       = av_strdup("out");
        _ctx_out.inout->filter_ctx = _ctx_out.ctx;
        _ctx_out.inout->pad_idx    = 0;
        _ctx_out.inout->next       = NULL;

        AVFilterInOut* inoutputs[2] = {_ctx_in_0.inout, _ctx_in_1.inout};

        ret = avfilter_graph_parse_ptr(_filter_graph, filter_desrc.c_str(), &_ctx_out.inout, inoutputs, NULL);
        if (ret < 0) {
            error = -1;
            break;
        }

        ret = avfilter_graph_config(_filter_graph, NULL);
        if (ret < 0) {
            error = -1;
            break;
        }

        //al_debug("dump graph:\r\n%s", avfilter_graph_dump(_filter_graph, NULL));

        _inited = true;
    } while (0);

    if (error != 0) {
        qCritical() << "filter init failed";
        cleanup();
    }

    //if (_ctx_in_0.inout)
    //	avfilter_inout_free(&_ctx_in_0.inout);

    //if (_ctx_in_1.inout)
    //	avfilter_inout_free(&_ctx_in_1.inout);

    //if (_ctx_out.inout)
    //	avfilter_inout_free(&_ctx_out.inout);

    return error;
}

int AmixFilter::start() {
    if (!_inited)
        return -1;

    if (_running)
        return 0;

    _running = true;
    _thread  = std::thread(std::bind(&AmixFilter::filter_loop, this));

    return 0;
}

int AmixFilter::stop() {
    if (!_inited || !_running)
        return 0;

    _running = false;

    _cond_notify = true;
    _cond_var.notify_all();

    if (_thread.joinable())
        _thread.join();

    return 0;
}

int AmixFilter::add_frame(AVFrame* frame, int index) {
    std::unique_lock<std::mutex> lock(_mutex);

    int error = 0;
    int ret   = 0;

    do {
        AVFilterContext* ctx = NULL;
        switch (index) {
            case 0:
                ctx = _ctx_in_0.ctx;
                break;
            case 1:
                ctx = _ctx_in_1.ctx;
                break;
            default:
                ctx = NULL;
                break;
        }

        if (!ctx) {
            error = -1;
            break;
        }

        ret = av_buffersrc_add_frame_flags(ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
        if (ret < 0) {
            error = -1;
            break;
        }

    } while (0);

    if (error != 0) {
        qCritical() << QString("av_buffersrc_add_frame_flags ret:%1,index:%2").arg(ret).arg(index);
    }

    _cond_notify = true;
    _cond_var.notify_all();

    return error;
}

const AVRational AmixFilter::get_time_base() {
    return av_buffersink_get_time_base(_ctx_out.ctx);
    return AVRational();
}

void AmixFilter::cleanup() {
    if (_filter_graph)
        avfilter_graph_free(&_filter_graph);

    memset(&_ctx_in_0, 0, sizeof(FILTER_CTX));
    memset(&_ctx_in_1, 0, sizeof(FILTER_CTX));
    memset(&_ctx_out, 0, sizeof(FILTER_CTX));

    _inited = false;
}

void AmixFilter::filter_loop() {
    AVFrame* sinkFrame       = av_frame_alloc();
    AVFrame* outFrame        = av_frame_alloc();
    outFrame->format         = _ctx_out.sample_fmt;
    outFrame->nb_samples     = 1024;  // TODO: 不要写死
    outFrame->channel_layout = _ctx_out.channel_layout;
    int ret                  = av_frame_get_buffer(outFrame, 0);
    if (ret < 0) {
        qCritical() << "av_frame_get_buffer failed";
        return;
    }

    // TODO: 不要写死
    m_filteredFrameFifo = av_audio_fifo_alloc(_ctx_out.sample_fmt, _ctx_out.nb_channel, 1024 * 5);
    if (!m_filteredFrameFifo) {
        qCritical() << "Fifo null";
        return;
    }

    ret = 0;
    while (_running) {
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_cond_notify && _running)
            _cond_var.wait_for(lock, std::chrono::milliseconds(300));

        while (_running && _cond_notify) {
            ret = av_buffersink_get_frame(_ctx_out.ctx, sinkFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }

            if (ret < 0) {
                qCritical() << "av_buffersink_get_frame failed:" << ret;
                if (_on_filter_error) _on_filter_error(ret, -1);
                break;
            }

            int space = av_audio_fifo_space(m_filteredFrameFifo);
            if (space < sinkFrame->nb_samples) {
                qCritical() << "fifo space is not enough";
                return;
            }
            int nbsamples = av_audio_fifo_write(m_filteredFrameFifo, (void**)sinkFrame->data, sinkFrame->nb_samples);
            if (nbsamples != sinkFrame->nb_samples) {
                qCritical() << "nbsamples != filt_frame->nb_samples";
                return;
            }

            av_frame_unref(sinkFrame);

            while (av_audio_fifo_size(m_filteredFrameFifo) >= outFrame->nb_samples) {
                int nbread = av_audio_fifo_read(m_filteredFrameFifo, (void**)outFrame->data, outFrame->nb_samples);
                if (nbread != outFrame->nb_samples) {
                    qCritical() << "av_audio_fifo_read failed";
                    return;
                }
                if (_on_filter_data)
                    _on_filter_data(outFrame, -1);
            }
        }

        _cond_notify = false;
    }

    av_audio_fifo_free(m_filteredFrameFifo);
    av_frame_free(&sinkFrame);
    av_frame_free(&outFrame);
    qInfo() << "AmixFilter thread exit";
}

}  // namespace onlyet