#ifndef ONLYET_AMIXFILTER_H
#define ONLYET_AMIXFILTER_H

#include "filter.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class AVFilterGraph;
class AVAudioFifo;

class AmixFilter {
public:
    AmixFilter();
    ~AmixFilter();

    int init(const FILTER_CTX &ctx_in0, const FILTER_CTX &ctx_in1, const FILTER_CTX &ctx_out);

    inline void registe_cb(on_filter_data cb_on_filter_data/*, on_filter_error cb_on_filter_error*/) {
        _on_filter_data  = cb_on_filter_data;
        //_on_filter_error = cb_on_filter_error;
    }

    int start();

    int stop();

    int add_frame(AVFrame *frame, int index);

    const AVRational get_time_base();

private:
    void cleanup();
    void filter_loop();

private:
    FILTER_CTX _ctx_in_0;
    FILTER_CTX _ctx_in_1;
    FILTER_CTX _ctx_out;

    AVFilterGraph *_filter_graph = nullptr;

    on_filter_data  _on_filter_data;
    on_filter_error _on_filter_error;

    std::atomic_bool _inited;
    std::atomic_bool _running;

    std::thread _thread;

    std::mutex              _mutex;
    std::condition_variable _cond_var;
    bool                    _cond_notify;

    AVAudioFifo *m_filteredFrameFifo = nullptr; // �����������ʱ����more samples than frame size
};

#endif  // !ONLYET_AMIXFILTER_H
