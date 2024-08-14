QT += widgets charts multimedia

TARGET = ToneGenerator
TEMPLATE = app

CONFIG += c++17

SOURCES += main.cpp

HEADERS +=

INCLUDEPATH += /usr/include/AL /Users/macbook2015/Downloads/SDL-release-2.30.6/include

LIBS += -L/Users/macbook2015/Downloads/SDL-release-2.30.6/build

