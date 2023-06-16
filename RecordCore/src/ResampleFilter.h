#ifndef ONLYET_RESAMPLEFILTER
#define ONLYET_RESAMPLEFILTER

#include "filter.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class AVFilterGraph;

namespace onlyet {

class ResampleFilter {
public:
    ResampleFilter();
    ~ResampleFilter();

    int init(const FILTER_CTX& ctx_in, const FILTER_CTX& ctx_out);

    inline void setFilterFrameCb(FilterFrameCb cb_on_filter_data) {
        m_filteredFrameCb = cb_on_filter_data;
    }

    int start();
    int stop();

    int addFrame(AVFrame* frame);
    const AVRational get_time_base();

private:
    void cleanup();
    void filterThread();

private:
    FILTER_CTX              m_input;
    FILTER_CTX              m_output;
    AVFilterGraph*          m_filterGraph = nullptr;
    FilterFrameCb          m_filteredFrameCb;
    std::atomic_bool        m_inited;
    std::atomic_bool        m_running;
    std::thread             m_thread;
    std::mutex              m_mutex;
    std::condition_variable m_condVar;
    bool                    m_gotNewFrame;
};

}  // namespace onlyet

#endif  // !ONLYET_RESAMPLEFILTER
