#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "QTimer"
#include "QListWidgetItem"

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
    xl::LogStatusFile * status_file = nullptr;
    void update_combined_checkboxes();

    void initialize_from_status_file();
private slots:
    void on_subjectList_itemChanged(QListWidgetItem * item);
    void on_levelList_itemChanged(QListWidgetItem * item);

    void on_allLevels_stateChanged(int i);
    void on_allSubjects_stateChanged(int i);

    void check_status_file_for_updates();
};

#endif // MAINWINDOW_H
