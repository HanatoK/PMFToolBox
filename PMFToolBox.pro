QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++2a

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    base/helper.cpp \
    base/histogram.cpp \
    base/reweighting.cpp \
    main.cpp \
    mainwindow.cpp \
    projectpmftab/projectpmf.cpp \
    reweightingtab/reweightingtab.cpp \
    reweightingtab/listmodeltrajectory.cpp \
    reweightingtab/tablemodelreweightingaxis.cpp

HEADERS += \
    base/helper.h \
    base/histogram.h \
    base/reweighting.h \
    mainwindow.h \
    projectpmftab/projectpmf.h \
    reweightingtab/reweightingtab.h \
    reweightingtab/listmodeltrajectory.h \
    reweightingtab/tablemodelreweightingaxis.h

FORMS += \
    mainwindow.ui \
    projectpmftab/projectpmf.ui \
    reweightingtab/reweightingtab.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
