TEMPLATE = app

QT += qml quick widgets

SOURCES += main.cpp \
    ../../ChessCore/Board.cpp \
    ../../ChessCore/ChessException.cpp \
    ../../ChessCore/Event.cpp \
    ../../ChessCore/Game.cpp \
    ../../ChessCore/Move.cpp \
    ../../ChessCore/Piece.cpp \
    ../../ChessCore/stdafx.cpp \
    chessconnector.cpp \
    ../../ChessCore/HistoryPlayer.cpp \
    ../../ChessCore/BoardPositionsCache.cpp

RESOURCES += qml.qrc

CONFIG += c++11
INCLUDEPATH += ../../ChessCore/

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

SUBDIRS += \
    ../../QtChessCoreLib/QtChessCoreLib/QtChessCoreLib.pro

HEADERS += \
    ../../ChessCore/Board.h \
    ../../ChessCore/ChessException.h \
    ../../ChessCore/Events.h \
    ../../ChessCore/Game.h \
    ../../ChessCore/Move.h \
    ../../ChessCore/Piece.h \
    ../../ChessCore/Serializer.h \
    ../../ChessCore/stdafx.h \
    ../../ChessCore/targetver.h \
    chessconnector.h \
    ../../ChessCore/HistoryPlayer.h \
    ../../ChessCore/BoardPositionsCache.h \
    ../../ChessCore/LruCacheMap.hpp
