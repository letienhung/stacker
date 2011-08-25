# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Add-in.
# ------------------------------------------------------

TEMPLATE = app
TARGET = Workspace
DESTDIR = ./Release
QT += core gui multimedia xml script opengl
CONFIG += release console
DEFINES += QT_LARGEFILE_SUPPORT QT_MULTIMEDIA_LIB QT_XML_LIB QT_OPENGL_LIB QT_SCRIPT_LIB _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES QT_DLL
INCLUDEPATH += ./GeneratedFiles \
    ./GeneratedFiles/$(Configuration) \
    . \
    ./libQGLViewer \
    ./GUI \
    ./GraphicsLibrary \
    ./Utility
LIBS += -L"./libQGLViewer/QGLViewer/lib" \
    -L"./GL" \
    -L"./OpenMesh/lib" \
    -lGLee \
    -lQGLViewer2 \
    -lopengl32 \
    -lglu32
PRECOMPILED_HEADER = StdAfx.h
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/release
OBJECTS_DIR += release
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(Workspace.pri)
win32:RC_FILE = Workspace.rc
