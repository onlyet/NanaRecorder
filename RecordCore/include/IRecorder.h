#ifndef ONLYET_IRECORDER_H
#define ONLYET_IRECORDER_H

#ifdef WIN32
#ifdef RECORDER_EXPORT
#define RECORDAPI __declspec(dllexport)
#else
#define RECORDAPI __declspec(dllimport)
#endif  // RECORDER_EXPORT
#else
#define RECORDAPI
#endif  // WIN32

#include <QVariant>
#include <qdebug.h>
#include <memory>

namespace onlyet {

class RECORDAPI IRecorder {
public:
    //IRecorder(){}
    virtual ~IRecorder() {}

    virtual void setRecordInfo(const QVariantMap& recordInfo) = 0;
    virtual int  startRecord()                                = 0;
    virtual int  pauseRecord()                                = 0;
    virtual int  resumeRecord()                               = 0;
    virtual int  stopRecord()                                 = 0;
};

RECORDAPI std::unique_ptr<IRecorder> createRecorder(const QVariantMap& recordInfo);

}  // namespace onlyet

#endif  // !ONLYET_IRECORDER_H
