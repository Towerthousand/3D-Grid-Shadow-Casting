QT -= gui

TARGET = 3D-Grid-Shadow-Casting
CONFIG -= app_bundle

TEMPLATE = app

include(../VBE-Scenegraph/VBE-Scenegraph.pri)
include(../VBE-Profiler/VBE-Profiler.pri)
include(../VBE/VBE.pri)

LIBS += -lGLEW -lGL -lSDL2
QMAKE_CXXFLAGS += -std=c++0x -fno-exceptions

INCLUDEPATH += .

SOURCES += \
    main.cpp \
    Scene.cpp \
    Grid.cpp \
    GridOrtho.cpp \
    Manager.cpp

HEADERS += \
    commons.hpp \
    Scene.hpp \
    Grid.hpp \
    GridOrtho.hpp \
    Manager.hpp

# OTHER_FILES += \
