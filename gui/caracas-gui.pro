# Caracas GUI QT project file

CONFIG += debug
TEMPLATE = app
TARGET = caracas-gui
INCLUDEPATH += .
QT += core gui widgets
LIBS += -lmarblewidget-qt5

# Input
HEADERS = mainscreen.hpp mapscreen.hpp diagnosticscreen.hpp
SOURCES = mainscreen.cpp mapscreen.cpp diagnosticscreen.cpp main.cpp

# Install
caracas-gui.path = /usr/local/src/
caracas-gui.files = caracas-gui
INSTALLS += caracas-gui
