QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++2a qwt

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    base/helper.cpp \
    base/histogram.cpp \
    base/historyfile.cpp \
    base/plot.cpp \
    base/reweighting.cpp \
    historypmftab/historypmftab.cpp \
    main.cpp \
    mainwindow.cpp \
    projectpmftab/projectpmftab.cpp \
    reweightingtab/listmodelfilelist.cpp \
    reweightingtab/reweightingtab.cpp \
    reweightingtab/tablemodelreweightingaxis.cpp

HEADERS += \
    base/helper.h \
    base/histogram.h \
    base/historyfile.h \
    base/plot.h \
    base/reweighting.h \
    base/turbocolormap.h \
    historypmftab/historypmftab.h \
    mainwindow.h \
    projectpmftab/projectpmftab.h \
    reweightingtab/listmodelfilelist.h \
    reweightingtab/reweightingtab.h \
    reweightingtab/tablemodelreweightingaxis.h

FORMS += \
    historypmftab/historypmftab.ui \
    mainwindow.ui \
    projectpmftab/projectpmftab.ui \
    reweightingtab/reweightingtab.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
