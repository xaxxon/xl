#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qfiledialog.h>


#include <xl/log.h>
using namespace xl::log;

// 0 if all not checked (or empty), 1 if mixed checked, 2 if all checked
int are_all_boxes_checked(QListWidget const & list_widget) {
    int all_checked = -1;
    for (int i = 0; i < list_widget.count(); i++) {
        if (list_widget.item(i)->checkState() == Qt::CheckState::Checked) {
            if (all_checked == -1) {
                all_checked = 2;
            } else if (all_checked == 0) {
                all_checked = 1;
            }
        } else {
            if (all_checked == -1) {
                all_checked = 0;
            } else if (all_checked == 2) {
                all_checked = 1;
            }
        }
    }
    if (all_checked == -1) {
        all_checked = 0;
    }
    return all_checked;
}


std::unique_ptr<LogStatusFileGuiWrapper> MainWindow::make_log_status_file_gui_wrapper(QString filename) {
    return std::make_unique<LogStatusFileGuiWrapper>(this->filename,
                                                     this->ui->levelList, this->ui->allLevels,
                                                     this->ui->subjectList, this->ui->allSubjects);
}

MainWindow::MainWindow(QString input_filename, QWidget *parent) :
    filename(std::move(input_filename)),
    timer(this),
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    log_status_file_gui_wrapper = this->make_log_status_file_gui_wrapper(this->filename);

    this->filename_label = new QLabel(this->filename);
    this->ui->statusBar->addPermanentWidget(this->filename_label);

    initialize_from_status_file();
    this->update_combined_checkboxes();
}


MainWindow::~MainWindow()
{
    delete ui;
}




void MainWindow::on_action_Open_triggered()
{
    this->filename = QFileDialog::getOpenFileName(this,
        tr("Open Status File"), ".", tr("Image Files (*.log_status)")
    );
    this->filename_label->setText(this->filename);
    log_status_file_gui_wrapper.reset(); // destroy old connections before creating new ones
    log_status_file_gui_wrapper = this->make_log_status_file_gui_wrapper(this->filename);
}

















