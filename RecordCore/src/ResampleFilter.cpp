#include <chrono>
#include <sstream>

#include "ResampleFilter.h"
#include "FFmpegHeader.h"

#include <QDebug>

namespace onlyet {

ResampleFilter::ResampleFilter() {
    memset(&m_input, 0, sizeof(FILTER_CTX));
    memset(&m_output, 0, sizeof(FILTER_CTX));

    m_filterGraph = NULL;

    m_inited  = false;
    m_running = false;

    m_gotNewFrame = false;
}

ResampleFilter::~ResampleFilter() {
    stop();
    cleanup();
}

int ResampleFilter::init(const FILTER_CTX& ctx_in, const FILTER_CTX& ctx_out) {
    int error = 0;
    int ret   = 0;

    if (m_inited) return error;

    do {
        m_input  = ctx_in;
        m_output = ctx_out;

        m_filterGraph = avfilter_graph_alloc();
        if (!m_filterGraph) {
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

        m_input.inout  = avfilter_inout_alloc();
        m_output.inout = avfilter_inout_alloc();

        char pad_args[512] = {0};

        format_pad_arg(pad_args, 512, m_input);

        ret = avfilter_graph_create_filter(&m_input.ctx, avfilter_get_by_name("abuffer"), "in", pad_args, NULL, m_filterGraph);
        if (ret < 0) {
            error = -1;
            break;
        }

        ret = avfilter_graph_create_filter(&m_output.ctx, avfilter_get_by_name("abuffersink"), "out", NULL, NULL, m_filterGraph);
        if (ret < 0) {
            error = -1;
            break;
        }

        av_opt_set_bin(m_output.ctx, "sample_fmts", (uint8_t*)&m_output.sample_fmt, sizeof(m_output.sample_fmt), AV_OPT_SEARCH_CHILDREN);
        av_opt_set_bin(m_output.ctx, "channel_layouts", (uint8_t*)&m_output.channel_layout, sizeof(m_output.channel_layout), AV_OPT_SEARCH_CHILDREN);
        av_opt_set_bin(m_output.ctx, "sample_rates", (uint8_t*)&m_output.sample_rate, sizeof(m_output.sample_rate), AV_OPT_SEARCH_CHILDREN);

        m_input.inout->name       = av_strdup("in");
        m_input.inout->filter_ctx = m_input.ctx;
        m_input.inout->pad_idx    = 0;
        m_input.inout->next       = NULL;

        m_output.inout->name       = av_strdup("out");
        m_output.inout->filter_ctx = m_output.ctx;
        m_output.inout->pad_idx    = 0;
        m_output.inout->next       = NULL;

        ret = avfilter_graph_parse_ptr(m_filterGraph, filter_desrc.c_str(), &m_output.inout, &m_input.inout, NULL);
        if (ret < 0) {
            error = -1;
            break;
        }

        ret = avfilter_graph_config(m_filterGraph, NULL);
        if (ret < 0) {
            error = -1;
            break;
        }

        m_inited = true;
    } while (0);

    if (error != 0) {
        qCritical() << "filter init failed";
        cleanup();
    }

    if (m_input.inout) avfilter_inout_free(&m_input.inout);
    if (m_output.inout) avfilter_inout_free(&m_output.inout);

    return error;
}

int ResampleFilter::start() {
    if (!m_inited)
        return -1;

    if (m_running)
        return 0;

    m_running = true;
    m_thread  = std::thread(std::bind(&ResampleFilter::filterThread, this));

    return 0;
}

int ResampleFilter::stop() {
    if (!m_inited || !m_running)
        return -1;

    m_running = false;

    m_gotNewFrame = true;
    m_condVar.notify_all();

    if (m_thread.joinable())
        m_thread.join();

    return 0;
}

int ResampleFilter::addFrame(AVFrame* frame) {
    int ret   = 0;

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        int                          ret = av_buffersrc_add_frame_flags(m_input.ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
    }
    if (ret < 0) {
        qCritical() << QString("av_buffersrc_add_frame_flags ret:%1").arg(ret);
    } else {
        m_gotNewFrame = true;
        m_condVar.notify_all();
    }

    return ret;
}

const AVRational ResampleFilter::get_time_base() {
    return av_buffersink_get_time_base(m_output.ctx);
}

void ResampleFilter::cleanup() {
    if (m_filterGraph)
        avfilter_graph_free(&m_filterGraph);

    memset(&m_input, 0, sizeof(FILTER_CTX));
    memset(&m_output, 0, sizeof(FILTER_CTX));

    m_inited = false;
}

void ResampleFilter::filterThread() {
    int ret = 0;
    AVFrame* sinkFrame = av_frame_alloc();

    while (1) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condVar.wait(lock, [this] { return !m_running || m_gotNewFrame; });
            if (!m_running) break;
        }
        ret = av_buffersink_get_frame(m_output.ctx, sinkFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            continue;
        } else if (ret < 0) {
            qCritical() << "av_buffersink_get_frame failed:" << ret;
            break;
        }

        if (m_filteredFrameCb)
            m_filteredFrameCb(sinkFrame, 0);

        av_frame_unref(sinkFrame);
    }

    av_frame_free(&sinkFrame);
    qInfo() << "ResampleFilter thread exit";
}

}  // namespace onlyet