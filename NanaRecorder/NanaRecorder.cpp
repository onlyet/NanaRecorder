#include "NanaRecorder.h"
#include <IRecorder.h>

#include <AppData.h>
#include <util.h>

#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QDesktopServices>

NanaRecorder::NanaRecorder(QWidget *parent)
    : QMainWindow(parent)
{
    initUI();
    ui.pauseBtn->hide();
}

NanaRecorder::~NanaRecorder() {
    qDebug() << "~NanaRecorder()";
}

void NanaRecorder::startBtnClicked() {
    // 暂停后恢复
    if (m_paused) {
        if (!m_recorder) return;

        m_paused = false;
        m_recorder->resumeRecord();

        m_recordTimer->start(1000);
        ui.pauseBtn->show();
        ui.startBtn->hide();
        //ui.infoFrame->setEnabled(false);

        qInfo() << "Resume record";
        return;
    }
    if (m_started) {
        return;
    }
    m_started = true;

    if (!m_recorder) {
        QString     c = ui.channelComboBox->currentText();
        QVariantMap info;
        QString     resolution = ui.resolutionComboBox->currentText();
        QStringList sl         = resolution.split('x');
        if (2 != sl.length()) {
            return;
        }
        info.insert("outWidth", sl.first());
        info.insert("outHeight", sl.last());
        info.insert("fps", ui.fpsComboBox->currentText());

        bool enableAudio = ui.audioCheckBox->isChecked();
        info.insert("enableAudio", enableAudio);
        if (enableAudio) {
            info.insert("audioDeviceIndex", ui.audioComboBox->currentIndex());
            info.insert("channel", ui.channelComboBox->currentText());
        }

        QString path = QString("%1/%2.mp4").arg(APPDATA->get(AppDataRole::RecordDir).toString(), util::currentDateTimeString("yyyy-MM-dd hh-mm-ss"));
        APPDATA->set(AppDataRole::RecordPath, path);
        info.insert("recordPath", path);
        m_recorder = createRecorder(info);
    }
    m_recorder->startRecord();

    m_recordDuration = 0;
    ui.durationLabel->setText("00:00:00");
    m_recordTimer->start(1000);
    ui.pauseBtn->show();
    ui.startBtn->hide();
    ui.infoFrame->setEnabled(false);

    qInfo() << "Start record";
}

void NanaRecorder::pauseBtnClicked() {
    if (!m_started || m_paused || !m_recorder) {
        return;
    }
    m_paused = true;

    if (m_recorder) {
        m_recorder->pauseRecord();
    }
    ui.startBtn->show();
    ui.pauseBtn->hide();
    m_recordTimer->stop();

    qInfo() << "Pause record";
}

void NanaRecorder::stopBtnClicked() {
    if (!m_started || !m_recorder) {
        return;
    }
    m_started = false;

    m_recorder->stopRecord();
    ui.startBtn->show();
    ui.pauseBtn->hide();
    m_recordTimer->stop();
    if (m_recorder) {
        //delete m_recorder;
        //m_recorder = nullptr;
        m_recorder.reset();
    }
    ui.infoFrame->setEnabled(true);

    bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(APPDATA->get(AppDataRole::RecordPath).toString()));
    if (!ok) {
        qCritical() << "QDesktopServices::openUrl failed";
    }
    qInfo() << "Stop record";
}

//void NanaRecorder::updateTime()
//{
//    static QDateTime dt;
//    dt = QDateTime::currentDateTime();
//    ui.timeLabel->setText(dt.toString("yyyy-MM-dd hh:mm:ss.zzz"));
//}

void NanaRecorder::updateRecordTime() {
    m_recordDuration += 1;
    int     hour         = m_recordDuration / 3600;
    QString hourString   = hour < 10 ? QString("0%1").arg(hour) : QString::number(hour);
    int     min          = m_recordDuration % 3600 / 60;
    QString minString    = min < 10 ? QString("0%1").arg(min) : QString::number(min);
    int     second       = m_recordDuration % 60;
    QString secondString = second < 10 ? QString("0%1").arg(second) : QString::number(second);
    ui.durationLabel->setText(QString("%1:%2:%3").arg(hourString).arg(minString).arg(secondString));
}

void NanaRecorder::initUI() {
    ui.setupUi(this);

    connect(ui.startBtn, &QPushButton::clicked, this, &NanaRecorder::startBtnClicked);
    connect(ui.pauseBtn, &QPushButton::clicked, this, &NanaRecorder::pauseBtnClicked);
    connect(ui.stopBtn, &QPushButton::clicked, this, &NanaRecorder::stopBtnClicked);

    //m_timer = new QTimer(this);
    //connect(m_timer, &QTimer::timeout, this, &NanaRecorder::updateTime);
    //m_timer->start(1000);

    m_recordTimer = new QTimer(this);
    connect(m_recordTimer, &QTimer::timeout, this, &NanaRecorder::updateRecordTime);
    ui.durationLabel->setText("00:00:00");
    //qInfo() << "av_version_info:" << av_version_info();

    ui.videoCheckBox->setEnabled(false);
    //ui.audioComboBox->setEnabled(false);
    //ui.channelComboBox->setEnabled(false);
    connect(ui.audioCheckBox, &QCheckBox::stateChanged, [this](int state) {
        bool checked = ui.audioCheckBox->isChecked();
        ui.audioComboBox->setEnabled(checked);
        ui.channelComboBox->setEnabled(checked);
    });

    QString dir = APPDATA->get(AppDataRole::RecordDir).toString();
    ui.recordEdit->setText(dir);
    connect(ui.recordPathBtn, &QPushButton::clicked, [this, dir]() {
        QFileDialog fileDialog;
        fileDialog.setWindowTitle(QStringLiteral("设置视频保存路径"));
        fileDialog.setDirectory(dir);
        fileDialog.setFileMode(QFileDialog::Directory);
        fileDialog.setViewMode(QFileDialog::Detail);
        if (!fileDialog.exec()) {
            return;
        }
        QStringList sl = fileDialog.selectedFiles();
        if (!sl.empty()) {
            QString newDir = sl.first();
            ui.recordEdit->setText(newDir);
            ui.recordEdit->setToolTip(newDir);
            APPDATA->set(AppDataRole::RecordDir, newDir);
        }
    });
}
