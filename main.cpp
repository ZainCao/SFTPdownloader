#include "mainwindow.h"
#include <QApplication>
#include <stdio.h>
#include <iostream>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    //std::cout<<"start."<<endl;
    w.setWindowIcon(QIcon(":/Icon/image/downloader.png"));
    w.show();
    return a.exec();
}
