#include <chrono>

#include "AmixFilter.h"
#include "FFmpegHeader.h"

#include <QDebug>

namespace onlyet {

AmixFilter::AmixFilter() {
    memset(&m_input0, 0, sizeof(FILTER_CTX));
    memset(&m_input1, 0, sizeof(FILTER_CTX));
    memset(&m_output, 0, sizeof(FILTER_CTX));

    m_filterGraph = NULL;
    m_inited       = false;
    m_running      = false;
    m_gotNewFrame  = false;
}

AmixFilter::~AmixFilter() {
    stop();
    cleanup();
}

int AmixFilter::init(const FILTER_CTX& intput0, const FILTER_CTX& input1, const FILTER_CTX& output) {
    int error = 0;
    int ret   = 0;

    if (m_inited) return error;

    do {
        m_input0  = intput0;
        m_input1 = input1;
        m_output  = output;

        m_filterGraph = avfilter_graph_alloc();
        if (!m_filterGraph) {
            error = -1;
            break;
        }

        const std::string filter_desrc = "[in0][in1]amix=inputs=2:duration=first:dropout_transition=0[out]";

        m_input0.inout = avfilter_inout_alloc();
        m_input1.inout = avfilter_inout_alloc();
        m_output.inout  = avfilter_inout_alloc();

        char pad_args0[512] = {0}, pad_args1[512] = {0};

        format_pad_arg(pad_args0, 512, m_input0);
        format_pad_arg(pad_args1, 512, m_input1);

        ret = avfilter_graph_create_filter(&m_input0.ctx, avfilter_get_by_name("abuffer"), "in0", pad_args0, NULL, m_filterGraph);
        if (ret < 0) {
            error = -1;
            break;
        }

        ret = avfilter_graph_create_filter(&m_input1.ctx, avfilter_get_by_name("abuffer"), "in1", pad_args1, NULL, m_filterGraph);
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

        m_input0.inout->name       = av_strdup("in0");
        m_input0.inout->filter_ctx = m_input0.ctx;
        m_input0.inout->pad_idx    = 0;
        m_input0.inout->next       = m_input1.inout;

        m_input1.inout->name       = av_strdup("in1");
        m_input1.inout->filter_ctx = m_input1.ctx;
        m_input1.inout->pad_idx    = 0;
        m_input1.inout->next       = NULL;

        m_output.inout->name       = av_strdup("out");
        m_output.inout->filter_ctx = m_output.ctx;
        m_output.inout->pad_idx    = 0;
        m_output.inout->next       = NULL;

        ret = avfilter_graph_parse_ptr(m_filterGraph, filter_desrc.c_str(), &m_output.inout, &m_input0.inout, NULL);
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

    if (m_input0.inout) avfilter_inout_free(&m_input0.inout);
    if (m_output.inout) avfilter_inout_free(&m_output.inout);

    return error;
}

int AmixFilter::start() {
    if (!m_inited)
        return -1;

    if (m_running)
        return 0;

    m_running = true;
    m_thread  = std::thread(std::bind(&AmixFilter::filterThread, this));

    return 0;
}

int AmixFilter::stop() {
    if (!m_inited || !m_running)
        return 0;

    m_running = false;

    m_gotNewFrame = true;
    m_condVar.notify_all();

    if (m_thread.joinable())
        m_thread.join();

    return 0;
}

int AmixFilter::addFrame(AVFrame* frame, int index) {
    int error = 0;
    int ret   = 0;

    do {
        AVFilterContext* ctx = NULL;
        switch (index) {
            case 0:
                ctx = m_input0.ctx;
                break;
            case 1:
                ctx = m_input1.ctx;
                break;
            default:
                ctx = NULL;
                break;
        }

        if (!ctx) {
            error = -1;
            break;
        }

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            ret = av_buffersrc_add_frame_flags(ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
        }
        if (ret < 0) {
            error = -1;
            break;
        }

    } while (0);

    if (error != 0) {
        qCritical() << QString("av_buffersrc_add_frame_flags ret:%1,index:%2").arg(ret).arg(index);
    } else {
        m_gotNewFrame = true;
        m_condVar.notify_one();
    }

    return error;
}

const AVRational AmixFilter::get_time_base() {
    return av_buffersink_get_time_base(m_output.ctx);
}

void AmixFilter::cleanup() {
    if (m_filterGraph)
        avfilter_graph_free(&m_filterGraph);

    memset(&m_input0, 0, sizeof(FILTER_CTX));
    memset(&m_input1, 0, sizeof(FILTER_CTX));
    memset(&m_output, 0, sizeof(FILTER_CTX));

    m_inited = false;
}

void AmixFilter::filterThread() {
    AVFrame* sinkFrame       = av_frame_alloc();
    AVFrame* outFrame        = av_frame_alloc();
    outFrame->format         = m_output.sample_fmt;
    outFrame->nb_samples     = 1024;  // TODO: 不要写死
    outFrame->channel_layout = m_output.channel_layout;
    int ret                  = av_frame_get_buffer(outFrame, 0);
    if (ret < 0) {
        qCritical() << "av_frame_get_buffer failed";
        return;
    }
    // TODO: 不要写死
    m_filteredFrameFifo = av_audio_fifo_alloc(m_output.sample_fmt, m_output.nb_channel, 1024 * 5);
    if (!m_filteredFrameFifo) {
        qCritical() << "Fifo null";
        return;
    }

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

        int space = av_audio_fifo_space(m_filteredFrameFifo);
        if (space < sinkFrame->nb_samples) {
            qCritical() << "fifo space is not enough";
            break;
        }
        int nbsamples = av_audio_fifo_write(m_filteredFrameFifo, (void**)sinkFrame->data, sinkFrame->nb_samples);
        if (nbsamples != sinkFrame->nb_samples) {
            qCritical() << "nbsamples != filt_frame->nb_samples";
            break;
        }

        av_frame_unref(sinkFrame);

        while (av_audio_fifo_size(m_filteredFrameFifo) >= outFrame->nb_samples) {
            int nbread = av_audio_fifo_read(m_filteredFrameFifo, (void**)outFrame->data, outFrame->nb_samples);
            if (nbread != outFrame->nb_samples) {
                qCritical() << "av_audio_fifo_read failed";
                return;
            }
            if (m_filteredFrameCb)
                m_filteredFrameCb(outFrame, -1);
        }
    }

    av_audio_fifo_free(m_filteredFrameFifo);
    av_frame_free(&sinkFrame);
    av_frame_free(&outFrame);
    qInfo() << "AmixFilter thread exit";
}

}  // namespace onlyet