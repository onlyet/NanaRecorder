#pragma once

#ifdef RECORDER_EXPORT
#define RECORDAPI __declspec(dllexport)
#else
#define RECORDAPI __declspec(dllimport)
#endif  // RECORDER_EXPORT

#include <QVariant>
#include <qdebug.h>
#include <memory>

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
//RECORDAPI IRecorder* freeRecorder();
