QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11
lessThan(QT_MAJOR_VERSION, 5): QMAKE_CXXFLAGS += -std=c++11

LIBS += -L/lib/aarch64-linux-gnu -lcurl
LIBS += -L/lib/aarch64-linux-gnu -lpaho-mqtt3c

TARGET = collect
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        qcustomplot.cpp\
        max30102.cpp\
        MaxPlot.cpp 

HEADERS  += qcustomplot.h\
            mainwindow.h\
            MaxPlot.h\
            max30102.h