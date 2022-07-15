#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_NanaRecorder.h"

class NanaRecorder : public QMainWindow
{
    Q_OBJECT

public:
    NanaRecorder(QWidget *parent = Q_NULLPTR);

private:
    Ui::NanaRecorderClass ui;
};
