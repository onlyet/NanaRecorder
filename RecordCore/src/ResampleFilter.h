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

    inline void registe_cb(on_filter_data cb_on_filter_data /*, on_filter_error cb_on_filter_error*/) {
        _on_filter_data = cb_on_filter_data;
        //_on_filter_error = cb_on_filter_error;
    }

    int start();

    int stop();

    int add_frame(AVFrame* frame);

    const AVRational get_time_base();

private:
    void cleanup();
    void filter_loop();

private:
    //int _index;

    FILTER_CTX _ctx_in;
    FILTER_CTX _ctx_out;

    AVFilterGraph* _filter_graph = nullptr;

    on_filter_data _on_filter_data;
    //on_filter_error _on_filter_error;

    std::atomic_bool _inited;
    std::atomic_bool _running;

    std::thread _thread;

    std::mutex              _mutex;
    std::condition_variable _cond_var;
    bool                    _cond_notify;
};

}  // namespace onlyet

#endif  // !ONLYET_RESAMPLEFILTER
