#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "QTimer"
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
    void update_combined_checkboxes();

    void initialize_from_status_file();

    QLabel * filename_label;

    std::unique_ptr<LogStatusFileGuiWrapper> log_status_file_gui_wrapper;


private slots:
    void on_subjectList_itemChanged(QListWidgetItem * item);
    void on_levelList_itemChanged(QListWidgetItem * item);

    void on_allLevels_stateChanged(int i);
    void on_allSubjects_stateChanged(int i);

    void check_status_file_for_updates();

    void on_action_Open_triggered();
};

#endif // MAINWINDOW_H
