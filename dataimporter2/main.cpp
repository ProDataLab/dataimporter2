#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("ProDataLab");
    QCoreApplication::setOrganizationDomain("prodatalab.com");
    QCoreApplication::setApplicationName("dataimporter2");


    MainWindow w;
    w.show();

    return a.exec();
}
