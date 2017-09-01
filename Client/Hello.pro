#-------------------------------------------------
#
# Project created by QtCreator 2017-08-20T17:16:14
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Hello
TEMPLATE = app
QMAKE_CXXFLAGS += -std=c++0x -g
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += MUDUO_STD_STRING
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    updown.cpp \
    client.cpp \
    md5.cpp

HEADERS += \
        mainwindow.h \
    updown.h \
    client.h \
    codec.h \
    md5.h

FORMS += \
        mainwindow.ui \
    updown.ui

RESOURCES += \
    resource.qrc
INCLUDEPATH += /usr/include/boost + /usr/include/muduo
LIBS           +=  -lmuduo_base_cpp11 -lmuduo_net_cpp11 -ljsoncpp -lpthread
