#include "mainwindow.h"
#include "ui_mainwindow.h"

#define XL_USE_PCRE
#include <xl/regex/regexer.h>

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
    this->ui->subjectList->clear();
    this->ui->levelList->clear();

    for(auto const & [name, status] : status_file->level_names) {
        auto item = new QListWidgetItem(name.c_str());
        item->setCheckState(status ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        ui->levelList->addItem(item);
    }

    for(auto const & [name, status] : status_file->subject_names) {
        auto item = new QListWidgetItem(name.c_str());
        item->setCheckState(status ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        ui->subjectList->addItem(item);
    }
}

void MainWindow::update_combined_checkboxes() {
    QSignalBlocker sb(this->ui->allSubjects);
    auto subject_result = are_all_boxes_checked(*this->ui->subjectList);
    this->ui->allSubjects->setCheckState((Qt::CheckState)subject_result);

    QSignalBlocker sb2(this->ui->allLevels);
    auto level_result = are_all_boxes_checked(*this->ui->levelList);
    this->ui->allLevels->setCheckState((Qt::CheckState)level_result);
}

MainWindow::MainWindow(QString filename, QWidget *parent) :
    filename(std::move(filename)),
    timer(this),
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->ui->statusBar->addPermanentWidget(new QLabel(this->filename));

    this->status_file = new xl::LogStatusFile(this->filename.toStdString());

    initialize_from_status_file();
    this->update_combined_checkboxes();

    connect(&this->timer, SIGNAL(timeout()), this, SLOT(check_status_file_for_updates()));
    this->timer.start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_levelList_itemChanged(QListWidgetItem * item) {

    for(auto & [name, status] : this->status_file->level_names) {
        if (name != item->text().toStdString()) {
            continue;
        }
        status = item->checkState() == Qt::CheckState::Checked;
    }
    status_file->write();
    this->ui->statusBar->showMessage("Saved configuration");

    QSignalBlocker(this);
    update_combined_checkboxes();

}


void MainWindow::on_subjectList_itemChanged(QListWidgetItem * item) {
    for(auto & [name, status] : this->status_file->subject_names) {
        if (name != item->text().toStdString()) {
            continue;
        }
        status = item->checkState() == Qt::CheckState::Checked;
    }

    status_file->write();
    this->ui->statusBar->showMessage("Saved configuration");

    QSignalBlocker(this);
    update_combined_checkboxes();

}

void MainWindow::check_status_file_for_updates() {
    static int count = 0;
//    this->ui->statusBar->showMessage(QString("checking: %1").arg(std::to_string(count++).c_str()));

    if (this->status_file->check()) {
        this->initialize_from_status_file();
    }
}

void MainWindow::on_allLevels_stateChanged(int i) {
    if (i != Qt::CheckState::PartiallyChecked) {
        this->ui->allLevels->setTristate(false);
    }
    bool const new_status = [&]{
    if (i == Qt::CheckState::Checked) {
        return true;
    } else if (i == Qt::CheckState::Unchecked) {
        return false;
    } else {
        return false;
    }
    }();
    for(auto & [name, status] : this->status_file->level_names) {
        status = new_status;
    }
    this->initialize_from_status_file();
    this->status_file->write();
}


void MainWindow::on_allSubjects_stateChanged(int i) {
    if (i != Qt::CheckState::PartiallyChecked) {
        this->ui->allSubjects->setTristate(false);
    }

    bool const new_status = [&]{
    if (i == Qt::CheckState::Checked) {
        return true;
    } else if (i == Qt::CheckState::Unchecked) {
        return false;
    } else {
        return false;
    }}();
    for(auto & [name, status] : this->status_file->subject_names) {
        status = new_status;
    }
    this->initialize_from_status_file();
    this->status_file->write();
}


