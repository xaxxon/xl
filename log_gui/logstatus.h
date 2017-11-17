#ifndef LOGSTATUS_H
#define LOGSTATUS_H

#include <QWidget>
#include <QTimer>

#include "../include/xl/log.h"
#include "logelementlistmodel.h"

namespace Ui {
class LogStatus;
}

class LogStatus : public QWidget
{
    Q_OBJECT

public:
    explicit LogStatus(QString const & filename, QWidget *parent = 0);
    ~LogStatus();

private:
    Ui::LogStatus *ui;


    std::unique_ptr<xl::log::LogStatusFile> status_file;

    LogElementListModel level_model;
    LogElementListModel subject_model;

    QTimer timer;


};

#endif // LOGSTATUS_H
