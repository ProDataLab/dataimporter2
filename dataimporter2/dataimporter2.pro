#-------------------------------------------------
#
# Project created by QtCreator 2016-03-26T09:20:48
#
#-------------------------------------------------

QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dataimporter2
TEMPLATE = app

CONFIG += warn_off

SOURCES += main.cpp\
        mainwindow.cpp \
    manager.cpp \
    ibhdf5.cpp

HEADERS  += mainwindow.h \
    manager.h \
    symbol.h \
    record.h \
    ibhdf5.h

FORMS    += mainwindow.ui

unix:!macx: LIBS += -L$$OUT_PWD/../ibqt/lib/ -libqt

INCLUDEPATH += $$PWD/../ibqt/lib
DEPENDPATH += $$PWD/../ibqt/lib



unix:!macx: LIBS += -L$$PWD/../../../../../../usr/lib/x86_64-linux-gnu/hdf5/serial/ -lhdf5

INCLUDEPATH += $$PWD/../../../../../../usr/lib/x86_64-linux-gnu/hdf5/serial/include
DEPENDPATH += $$PWD/../../../../../../usr/lib/x86_64-linux-gnu/hdf5/serial/include

unix:!macx: LIBS += -L$$PWD/../../../../../../usr/lib/x86_64-linux-gnu/hdf5/serial/lib/ -lhdf5_hl

INCLUDEPATH += $$PWD/../../../../../../usr/lib/x86_64-linux-gnu/hdf5/serial/include
DEPENDPATH += $$PWD/../../../../../../usr/lib/x86_64-linux-gnu/hdf5/serial/include
