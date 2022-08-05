#pragma once

#include <functional>


class FreeBug
{
public:
    FreeBug();
    void setCb(std::function<int(int)> cb) { m_cb = cb; }
    void run();
    int freeBug(int dummy);

    void cb();
    void call();

private:
    std::function<int(int)> m_cb;
    uint8_t* m_resampleBuf[8];

};

void av_freep_bug_test();

void test();


