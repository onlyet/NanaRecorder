#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_NanaRecorder.h"

//#include <QTime>

class Recorder;

class QTimer;

class NanaRecorder : public QMainWindow
{
    Q_OBJECT

public:
    NanaRecorder(QWidget *parent = Q_NULLPTR);

private slots:
    void startBtnClicked();
    void pauseBtnClicked();
    void stopBtnClicked();
    //void updateTime();
    void updateRecordTime();

    private:
    void initUI();

private:
    Ui::NanaRecorderClass ui;
    Recorder*             m_recorder = nullptr;
    //QTimer*               m_timer    = nullptr;
    QTimer*               m_recordTimer = nullptr;
    int                   m_totalTimeSec = 0;
    bool                  m_started      = false;
    bool                  m_paused       = false;
};
