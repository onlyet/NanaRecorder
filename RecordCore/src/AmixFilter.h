#ifndef ONLYET_AMIXFILTER_H
#define ONLYET_AMIXFILTER_H

#include "filter.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class AVFilterGraph;
class AVAudioFifo;

namespace onlyet {

class AmixFilter {
public:
    AmixFilter();
    ~AmixFilter();

    int init(const FILTER_CTX &intput0, const FILTER_CTX &input1, const FILTER_CTX &output);

    inline void setFilterFrameCb(FilterFrameCb filteredFrameCb) {
        m_filteredFrameCb = filteredFrameCb;
    }

    int start();
    int stop();

    int addFrame(AVFrame *frame, int index);
    const AVRational get_time_base();

private:
    void cleanup();
    void filterThread();

private:
    FILTER_CTX              m_input0;
    FILTER_CTX              m_input1;
    FILTER_CTX              m_output;
    AVFilterGraph*          m_filterGraph = nullptr;
    FilterFrameCb          m_filteredFrameCb;
    std::atomic_bool        m_inited;
    std::atomic_bool        m_running;
    std::thread             m_thread;
    std::mutex              m_mutex;
    std::condition_variable m_condVar;
    bool                    m_gotNewFrame;
    AVAudioFifo*            m_filteredFrameFifo = nullptr;  // 用作避免编码时报错：more samples than frame size
};

}  // namespace onlyet

#endif  // !ONLYET_AMIXFILTER_H
