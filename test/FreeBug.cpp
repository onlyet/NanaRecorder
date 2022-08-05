#include "FreeBug.h"

#include <qdebug.h>

#ifdef	__cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"

#ifdef __cplusplus
};
#endif

#include <iostream>

using namespace std;
using namespace std::placeholders;

FreeBug::FreeBug()
{
    memset(m_resampleBuf, 0, sizeof(m_resampleBuf));
}

void FreeBug::run()
{
    qDebug() << "cb 1";
    m_cb(1);
    int a = 3;
    qDebug() << "a = " << a;
    qDebug() << "cb 2";
}

int FreeBug::freeBug(int dummy)
{
    int dst_nb_samples = 500;

    for (int t = 0; t < 1000; ++t) {
        for (int i = 0; i < 2; ++i) {
            qDebug() << "dump 1-1-1";
            if (m_resampleBuf[i]) {
                av_freep(&m_resampleBuf[i]);
            }
            qDebug() << "dump 1-1-2";
        }

        int ret = av_samples_alloc(m_resampleBuf, NULL, 2, dst_nb_samples, AV_SAMPLE_FMT_FLTP, 0);
        if (ret < 0) {
            qDebug() << "av_samples_alloc failed";
            return -1;
        }
    }

    return 0;
}

void av_freep_bug_test() {
 //   FreeBug* fb = new FreeBug;

 //   //function<int(int)> cb = bind(&FreeBug::freeBug, fb, _1);
 //   //fb->setCb(cb);
 //   //fb->run();

 ///*   delete fb;
 //   delete fb;*/

 //   char* p = (char*)malloc(1000);
 //   free(p);
 //   free(p + 500);

}



void FreeBug::cb() {
    // ...
    char* p = (char*)malloc(1000);
    free(p);
    free(p + 500); // 这里出的bug
    // ...
    int a = 1;
    int b = 2;
    int sum = a + b;
    cout << "sum:" << sum << endl;
}
void FreeBug::call() {
    function<void(void)> cb = bind(&FreeBug::cb, this);
    cb();
    int s = 1 + 2; // 崩溃堆栈只显示到在这里，定位不到cb里具体哪一行
    cout << s;
}

void test()
{
    FreeBug* fb = new FreeBug;
    fb->call();
}



