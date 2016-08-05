#include "mainwindow.h"
#include "filecopyworker.h"
#include <QApplication>
#include <QMetaType>

int main(int argc, char *argv[])
{
    qRegisterMetaType<FileCopyInfo>("FileCopyInfo");
    qRegisterMetaType<QList<FileCopyInfo>>("FileCopyTask");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
