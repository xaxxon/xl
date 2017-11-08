#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if (argc < 2) {
        exit(0);
    }
    MainWindow w(argv[1]);
    w.show();

    return a.exec();
}
