LIBS += -ludev
QT += widgets
QMAKE_CXXFLAGS += -std=c++11

TEMPLATE = app
TARGET = hostware_qt
INCLUDEPATH += ../include/

# Input
HEADERS += fifo.h MainWindow.h parser.h qchardev.h qdrawboxwidget.h
FORMS += MainWindow.ui
SOURCES += fifo.cpp \
           main.cpp \
           MainWindow.cpp \
           parser.cpp \
           qchardev.cpp \
           qdrawboxwidget.cpp
