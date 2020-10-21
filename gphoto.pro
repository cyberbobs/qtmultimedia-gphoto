TARGET = gphoto

QT       += core gui multimedia
TEMPLATE =  lib
CONFIG   += plugin

PLUGIN_TYPE=mediaservice
PLUGIN_CLASS_NAME=GPhotoServicePlugin

#DESTDIR = $$[QT_INSTALL_PLUGINS]/mediaservice

SOURCES += \
    gphotocamera.cpp \
    gphotocameracapturedestinationcontrol.cpp \
    gphotocameracontrol.cpp \
    gphotocamerafocuscontrol.cpp \
    gphotocameraimagecapturecontrol.cpp \
    gphotocameralockcontrol.cpp \
    gphotocamerasession.cpp \
    gphotocontroller.cpp \
    gphotoexposurecontrol.cpp \
    gphotomediaservice.cpp \
    gphotoserviceplugin.cpp \
    gphotovideoinputdevicecontrol.cpp \
    gphotovideoprobecontrol.cpp \
    gphotovideorenderercontrol.cpp \
    gphotoworker.cpp

HEADERS += \
    gphotocamera.h \
    gphotocameracapturedestinationcontrol.h \
    gphotocameracontrol.h \
    gphotocamerafocuscontrol.h \
    gphotocameraimagecapturecontrol.h \
    gphotocameralockcontrol.h \
    gphotocamerasession.h \
    gphotocontroller.h \
    gphotoexposurecontrol.h \
    gphotomediaservice.h \
    gphotoserviceplugin.h \
    gphotovideoinputdevicecontrol.h \
    gphotovideoprobecontrol.h \
    gphotovideorenderercontrol.h \
    gphotoworker.h

OTHER_FILES += gphoto.json
LIBS += -lgphoto2

target.path = $$[QT_INSTALL_PLUGINS]/mediaservice
INSTALLS += target
