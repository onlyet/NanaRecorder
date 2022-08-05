#include "NanaRecorder.h"
#include "Recorder.h"
#include "FFmpegHeader.h"

#include <QTimer>
#include <QDateTime>
#include <QDebug>

NanaRecorder::NanaRecorder(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);


    connect(ui.startBtn, &QPushButton::clicked, this, &NanaRecorder::startBtnClicked);
    connect(ui.stopBtn, &QPushButton::clicked, this, &NanaRecorder::stopBtnClicked);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &NanaRecorder::updateTime);
    m_timer->start(1000);

    qDebug() << "av_version_info:" << av_version_info();
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

void NanaRecorder::updateTime()
{
    static QDateTime dt;
    dt = QDateTime::currentDateTime();
    ui.timeLabel->setText(dt.toString("yyyy-MM-dd hh:mm:ss.zzz"));
}
