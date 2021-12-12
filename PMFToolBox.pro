QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++2a qwt

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# You need to comment out USE_BOOST_* if you do not have boost installed.
#DEFINES += USE_BOOST_D_ARY_HEAP
#DEFINES += USE_BOOST_PRIORITY_QUEUQ
# Simply copying the priority queue and pop elements for comparison seems having better performance
# From boost documentation: "iterating the a heap in heap order has an amortized complexity of O(N*log(N))."
#DEFINES += USE_BOOST_ORDERED_ITERATOR

SOURCES += \
    aboutdialog/aboutdialog.cpp \
    base/cliobject.cpp \
    base/graph.cpp \
    base/helper.cpp \
    base/histogram.cpp \
    base/historyfile.cpp \
    base/metadynamics.cpp \
    base/pathfinderthread.cpp \
    base/plot.cpp \
    base/reweighting.cpp \
    findpathtab/addpatchdialog.cpp \
    findpathtab/findpathtab.cpp \
    findpathtab/patchtablemodel.cpp \
    historypmftab/historypmftab.cpp \
    main.cpp \
    mainwindow.cpp \
    base/namdlogparser.cpp \
    metadynamicstab/metadynamicstab.cpp \
    metadynamicstab/tablemodelaxes.cpp \
    namdlogtab/namdlogtab.cpp \
    namdlogtab/tablemodelbinning.cpp \
    projectpmftab/projectpmftab.cpp \
    reweightingtab/listmodelfilelist.cpp \
    reweightingtab/reweightingtab.cpp \
    reweightingtab/tablemodelreweightingaxis.cpp \
    test/test.cpp

HEADERS += \
    aboutdialog/aboutdialog.h \
    base/cliobject.h \
    base/common.h \
    base/graph.h \
    base/helper.h \
    base/histogram.h \
    base/historyfile.h \
    base/metadynamics.h \
    base/pathfinderthread.h \
    base/plot.h \
    base/reweighting.h \
    base/turbocolormap.h \
    findpathtab/addpatchdialog.h \
    findpathtab/findpathtab.h \
    findpathtab/patchtablemodel.h \
    historypmftab/historypmftab.h \
    mainwindow.h \
    base/namdlogparser.h \
    metadynamicstab/metadynamicstab.h \
    metadynamicstab/tablemodelaxes.h \
    namdlogtab/namdlogtab.h \
    namdlogtab/tablemodelbinning.h \
    projectpmftab/projectpmftab.h \
    reweightingtab/listmodelfilelist.h \
    reweightingtab/reweightingtab.h \
    reweightingtab/tablemodelreweightingaxis.h \
    test/test.h

FORMS += \
    aboutdialog/aboutdialog.ui \
    findpathtab/findpathtab.ui \
    historypmftab/historypmftab.ui \
    mainwindow.ui \
    metadynamicstab/metadynamicstab.ui \
    namdlogtab/namdlogtab.ui \
    projectpmftab/projectpmftab.ui \
    reweightingtab/reweightingtab.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
  text.qrc
