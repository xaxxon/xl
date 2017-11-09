#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    MainWindow w(argc > 1 ? argv[1] : "example.log_status");
    w.show();

    return a.exec();
}
