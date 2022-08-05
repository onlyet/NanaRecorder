#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_NanaRecorder.h"

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

private:
    Ui::NanaRecorderClass ui;
    Recorder* m_recorder = nullptr;
    QTimer* m_timer = nullptr;
};
