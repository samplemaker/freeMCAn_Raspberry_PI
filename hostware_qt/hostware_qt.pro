LIBS += -ludev
QT += widgets
QMAKE_CXXFLAGS += -std=c++11

TEMPLATE = app
TARGET = hostware_qt
INCLUDEPATH += ../include/

# Input
HEADERS += MainWindow.h parser.h qchardev.h
FORMS += MainWindow.ui
SOURCES += main.cpp MainWindow.cpp parser.cpp qchardev.cpp
