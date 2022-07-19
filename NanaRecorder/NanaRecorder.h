#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_NanaRecorder.h"

class Recorder;

class NanaRecorder : public QMainWindow
{
    Q_OBJECT

public:
    NanaRecorder(QWidget *parent = Q_NULLPTR);

private slots:
    void startBtnClicked();
    void stopBtnClicked();

private:
    Ui::NanaRecorderClass ui;
    Recorder* m_recorder = nullptr;
};
