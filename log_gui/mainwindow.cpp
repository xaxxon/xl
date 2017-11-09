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


void MainWindow::initialize_from_status_file() {
//    this->ui->subjectList->clear();
//    this->ui->levelList->clear();

//    for(auto const & [name, status] : status_file->level_names) {
//        auto item = new QListWidgetItem(name.c_str());
//        item->setCheckState(status ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
//        ui->levelList->addItem(item);
//    }

//    for(auto const & [name, status] : status_file->subject_names) {
//        auto item = new QListWidgetItem(name.c_str());
//        item->setCheckState(status ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
//        ui->subjectList->addItem(item);
//    }
}


void MainWindow::update_combined_checkboxes() {
//    QSignalBlocker sb(this->ui->allSubjects);
//    auto subject_result = are_all_boxes_checked(*this->ui->subjectList);
//    this->ui->allSubjects->setCheckState((Qt::CheckState)subject_result);

//    QSignalBlocker sb2(this->ui->allLevels);
//    auto level_result = are_all_boxes_checked(*this->ui->levelList);
//    this->ui->allLevels->setCheckState((Qt::CheckState)level_result);
}


MainWindow::MainWindow(QString input_filename, QWidget *parent) :
    filename(std::move(input_filename)),
    timer(this),
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    log_status_file_gui_wrapper =
            std::make_unique<LogStatusFileGuiWrapper>(this->filename,
                                                      this->ui->levelList, this->ui->allLevels,
                                                      this->ui->subjectList, this->ui->allSubjects);


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
//    this->filename = QFileDialog::getOpenFileName(this,
//        tr("Open Status File"), ".", tr("Image Files (*.log_status)")
//    );
////    this->statusBar()->showMessage(filename);
//    this->filename_label->setText(this->filename);
//    this->status_file = new xl::LogStatusFile(filename.toStdString());
}

















