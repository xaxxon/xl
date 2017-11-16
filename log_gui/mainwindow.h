#pragma once

#include "QLabel"
#include "QListWidgetItem"

#include "logelementlistmodel.h"

namespace xl {
class LogStatusFile;
}

#include <QMainWindow>

namespace Ui {
class MainWindow;
}



class MainWindow : public QMainWindow
{
    Q_OBJECT
    QString filename;
    QTimer timer;

public:
    explicit MainWindow(QString filename, QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;


    QLabel * filename_label;

    std::unique_ptr<LogStatusFileGuiWrapper> log_status_file_gui_wrapper;

    std::unique_ptr<LogStatusFileGuiWrapper> make_log_status_file_gui_wrapper(QString filename);


private slots:

    void on_action_Open_triggered();
};

