######################################################################
# Automatically generated by qmake (3.1) Thu Apr 13 09:47:21 2023
######################################################################

TEMPLATE = app
TARGET = mp4ToPng 
INCLUDEPATH += /usr/local/ffmpeg/include
LIBS += -L/usr/local/ffmpeg/lib -lavcodec -lavformat -lavutil -lswscale -lpng

# Input
HEADERS += qcutter.h
SOURCES += qcutter.cpp \
           main.cpp
