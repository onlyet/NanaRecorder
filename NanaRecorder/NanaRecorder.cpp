#include "NanaRecorder.h"
#include "Recorder.h"

NanaRecorder::NanaRecorder(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);


    connect(ui.startBtn, &QPushButton::clicked, this, &NanaRecorder::startBtnClicked);
    connect(ui.stopBtn, &QPushButton::clicked, this, &NanaRecorder::stopBtnClicked);
}

void NanaRecorder::startBtnClicked()
{
    if (!m_recorder) {
        m_recorder = new Recorder;
        m_recorder->setRecordInfo();
    }
    m_recorder->startRecord();
}

void NanaRecorder::stopBtnClicked()
{
    m_recorder->stopRecord();
    if (m_recorder) {
        delete m_recorder;
        m_recorder = nullptr;
    }
}
