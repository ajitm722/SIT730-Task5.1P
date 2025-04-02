# Include Qt core and GUI modules
QT += core gui

# If Qt version is greater than 4, also include the Qt Widgets module
# (Qt 5 and 6 have widgets separated from 'gui')
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Enable C++17 features in compiler
CONFIG += c++17

# The name of the final executable (task5.1GUI)
TARGET = task5.1GUI

# This is an application (not a library)
TEMPLATE = app

# Source files included in the build (only main.cpp in this case)
SOURCES += src/main.cpp

# Add system include path (useful for system-wide headers)
INCLUDEPATH += /usr/include

# Link with required non-Qt libraries:
# -lpigpio: pigpio library for GPIO control
# -lrt: real-time extensions (used by pigpio internally)
# -lpthread: POSIX threading (required by pigpio and Qt itself)
LIBS += -lpigpio -lrt -lpthread
