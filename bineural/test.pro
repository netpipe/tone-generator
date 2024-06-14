QT += widgets charts

TARGET = ToneGenerator
TEMPLATE = app

CONFIG += c++17

SOURCES += main.cpp

HEADERS +=

INCLUDEPATH += /usr/include/AL

LIBS += -lopenal

