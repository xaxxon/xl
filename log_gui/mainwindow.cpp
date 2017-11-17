#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qfiledialog.h>

#include "logstatus.h"
#include "../include/xl/log.h"
using namespace xl::log;



MainWindow::MainWindow(QString filename, QWidget *parent) :
    QMainWindow(parent),
    ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    auto tabs = ui->logStatusTabs;
    while(tabs->count() > 0) {
        tabs->removeTab(0);
    }

    tabs->addTab(new LogStatus(filename), filename);
}

MainWindow::~MainWindow() = default;



void MainWindow::on_action_Open_triggered()
{
    auto filename = QFileDialog::getOpenFileName(this,
        tr("Open Status File"), ".", tr("Image Files (*.log_status)")
    );

    this->ui->logStatusTabs->addTab(new LogStatus(filename), filename);

}

















