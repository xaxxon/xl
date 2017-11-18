#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QStringList args;
    for(int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }
    if (args.empty()) {
        args.push_back("./");
    }
    MainWindow w(args);
    w.show();

    return a.exec();
}


