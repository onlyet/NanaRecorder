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

    m_recordTimer = new QTimer(this);
    connect(m_recordTimer, &QTimer::timeout, this, &NanaRecorder::updateRecordTime);
    ui.durationLabel->setText("00:00:00");
    qDebug() << "av_version_info:" << av_version_info();

    ui.videoCheckBox->setEnabled(false);
}

void NanaRecorder::startBtnClicked()
{
    m_totalTimeSec = 0;
    ui.durationLabel->setText("00:00:00");
    m_recordTimer->start(1000);
    if (!m_recorder) {
        QVariantMap info;
        bool        enableAudio = ui.audioCheckBox->isChecked();
        info.insert("enableAudio", enableAudio);
        if (enableAudio) {
            info.insert("audioDeviceIndex", ui.audioComboBox->currentIndex());
            info.insert("channel", ui.channelComboBox->currentText());
        }
        m_recorder = new Recorder(info);
    }
    m_recorder->startRecord();
}

void NanaRecorder::stopBtnClicked()
{
    m_recordTimer->stop();
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

void NanaRecorder::updateRecordTime() {
    m_totalTimeSec += 1;
    int     hour         = m_totalTimeSec / 3600;
    QString hourString   = hour < 10 ? QString("0%1").arg(hour) : QString::number(hour);
    int     min          = m_totalTimeSec % 3600 / 60;
    QString minString    = min < 10 ? QString("0%1").arg(min) : QString::number(min);
    int     second       = m_totalTimeSec % 60;
    QString secondString = second < 10 ? QString("0%1").arg(second) : QString::number(second);
    ui.durationLabel->setText(QString("%1:%2:%3").arg(hourString).arg(minString).arg(secondString));
}
