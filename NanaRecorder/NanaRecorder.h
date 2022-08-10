#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_NanaRecorder.h"

#include <QTime>

class Recorder;

class QTimer;

class NanaRecorder : public QMainWindow
{
    Q_OBJECT

public:
    NanaRecorder(QWidget *parent = Q_NULLPTR);

private slots:
    void startBtnClicked();
    void stopBtnClicked();
    void updateTime();
    void updateRecordTime();

private:
    Ui::NanaRecorderClass ui;
    Recorder*             m_recorder = nullptr;
    QTimer*               m_timer    = nullptr;
    //QTime                 m_recordTime;
    QTimer*               m_recordTimer = nullptr;
    int                   m_totalTimeSec = 0;
};
