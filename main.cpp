#include "mainwindow.hpp"
#include <QApplication>
#include <modalwaitdialog.h>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
