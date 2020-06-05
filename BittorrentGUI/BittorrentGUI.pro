QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../src/Client.cpp \
    ../src/HTTPClient.cpp \
    ../src/Peer.cpp \
    ../src/Torrent.cpp \
    ../src/TorrentGeneral.cpp \
    ../src/TorrentHashes.cpp \
    ../src/TorrentPieces.cpp \
    ../src/TorrentStatus.cpp \
    ../src/UDPClient.cpp \
    ../src/encodeVisitor.cpp \
    ../src/loguru.cpp \
    ../src/sha1.cpp \
    ../src/tokenizer.cpp \
    ../src/trackerObj.cpp \
    ../src/trackerUrl.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ../include/ \
    ../include/Client.h \
    ../include/Decoder.h \
    ../include/HTTPClient.h \
    ../include/Peer.h \
    ../include/Tokenizer.h \
    ../include/Torrent.h \
    ../include/TorrentGeneral.h \
    ../include/TorrentHashes.h \
    ../include/TorrentManipulation.h \
    ../include/TorrentPieces.h \
    ../include/TorrentStatus.h \
    ../include/UDPClient.h \
    ../include/Utility.h \
    ../include/ValueTypes.h \
    ../include/encodeVisitor.h \
    ../include/fileObj.h \
    ../include/loguru.h \
    ../include/sha1.h \
    ../include/trackerObj.h \
    ../include/trackerUrl.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += ../include/
INCLUDEPATH += $$PWD/../../../Boostmingw/boost_1_73_0
INCLUDEPATH += $$PWD/../../../Boostmingw/boost_1_73_0/stage

DEPENDPATH += $$PWD/../../../Boostmingw/boost_1_73_0/stage


LIBS += -L$$PWD/../../../Boostmingw/boost_1_73_0/stage/lib/ -lboost_filesystem-mgw8-mt-x64-1_73
LIBS += -L$$PWD/../../../Boostmingw/boost_1_73_0/stage/lib/ -lboost_system-mgw8-mt-x64-1_73
LIBS += -L$$PWD/../../../Boostmingw/boost_1_73_0/stage/lib/ -lboost_serialization-mgw8-mt-x64-1_73
LIBS += -L$$PWD/../../../Boostmingw/boost_1_73_0/stage/lib/ -lboost_regex-mgw8-mt-x64-1_73
LIBS += -L$$PWD/../../../Boostmingw/boost_1_73_0/stage/lib/ -lboost_date_time-mgw8-mt-x64-1_73

#winsock2 lib
LIBS += -lWS2_32
LIBS += -lwsock32

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target



