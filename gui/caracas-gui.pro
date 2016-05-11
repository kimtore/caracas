# Caracas GUI QT project file

CONFIG += debug
TEMPLATE = app
TARGET = caracas-gui
INCLUDEPATH += . /usr/include/taglib
QT += core gui widgets svg
LIBS += -lmarblewidget-qt5 -lmpdclient -ltag

# Input
HEADERS = mainscreen.hpp mapscreen.hpp diagnosticscreen.hpp musicscreen.hpp mpdclient.hpp time.hpp tagfile.hpp albumartwidget.hpp playerscreen.hpp listscreen.hpp
SOURCES = mainscreen.cpp mapscreen.cpp diagnosticscreen.cpp main.cpp musicscreen.cpp mpdclient.cpp time.cpp tagfile.cpp albumartwidget.cpp playerscreen.cpp listscreen.cpp

# Install
caracas-gui.path = /usr/local/bin/
caracas-gui.files = caracas-gui
INSTALLS += caracas-gui
