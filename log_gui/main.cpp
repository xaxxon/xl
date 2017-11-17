#include "mainwindow.h"
#include <QApplication>
#include <fmt/format.h>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    std::cerr << fmt::format("argc: {}", argc) << std::endl;
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
