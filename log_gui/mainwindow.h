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

public:
    explicit MainWindow(QString filename, QWidget *parent = 0);
    ~MainWindow();

private:
    std::unique_ptr<Ui::MainWindow> ui;

private slots:

    void on_action_Open_triggered();
};

