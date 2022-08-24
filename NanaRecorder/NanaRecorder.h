#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_NanaRecorder.h"

#include <memory>

class IRecorder;

class QTimer;

class NanaRecorder : public QMainWindow
{
    Q_OBJECT

public:
    NanaRecorder(QWidget *parent = Q_NULLPTR);
    ~NanaRecorder();

private slots:
    void startBtnClicked();
    void pauseBtnClicked();
    void stopBtnClicked();
    //void updateTime();
    void updateRecordTime();

    private:
    void initUI();

private:
    Ui::NanaRecorderClass      ui;
    std::unique_ptr<IRecorder> m_recorder;
    QTimer*                    m_recordTimer    = nullptr;
    int                        m_recordDuration = 0;
    bool                       m_started        = false;
    bool                       m_paused         = false;
};
