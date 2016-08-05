#-------------------------------------------------
#
# Project created by QtCreator 2016-07-28T20:19:44
#
#-------------------------------------------------

QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VFCopy
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        qexifimageheader.cpp \
    filecopyworker.cpp

HEADERS  += mainwindow.h\
        qexifimageheader.h \
    filecopyworker.h

FORMS    += mainwindow.ui
