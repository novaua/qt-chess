TEMPLATE = app

QT += qml quick widgets multimedia

SOURCES += main.cpp \
    AppConfig.cpp \
    ../../ChessCore/Board.cpp \
    ../../ChessCore/BoardPositionsCache.cpp \
    ../../ChessCore/Check.cpp \
    ../../ChessCore/ChessException.cpp \
    ../../ChessCore/Events.cpp \
    ../../ChessCore/Game.cpp \
    ../../ChessCore/HistoryPlayer.cpp \
    ../../ChessCore/Move.cpp \
    ../../ChessCore/Piece.cpp \
    ../../ChessCore/stdafx.cpp \
    ../../UciConnector/UciConnector.cpp \
    ../../UciConnector/pch.cpp \
    chessconnector.cpp \
    engineworker.cpp \
    ChessEnginePlayer.cpp

RESOURCES += qml.qrc

CONFIG += c++11
INCLUDEPATH += ../../ChessCore/ ../../UciConnector/

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = .

# Default rules for deployment.
include(deployment.pri)

SUBDIRS += \
    ../../QtChessCoreLib/QtChessCoreLib/QtChessCoreLib.pro

HEADERS += \
    AppConfig.h \
    ../../ChessCore/Board.h \
    ../../ChessCore/BoardPositionsCache.h \
    ../../ChessCore/Check.h \
    ../../ChessCore/ChessException.h \
    ../../ChessCore/Events.h \
    ../../ChessCore/Game.h \
    ../../ChessCore/HistoryPlayer.h \
    ../../ChessCore/LruCacheMap.hpp \
    ../../ChessCore/Move.h \
    ../../ChessCore/Piece.h \
    ../../ChessCore/Serializer.h \
    ../../ChessCore/stdafx.h \
    ../../ChessCore/targetver.h \
    ../../UciConnector/UciConnector.h \
    ../../UciConnector/framework.h \
    ../../UciConnector/pch.h \
    chessconnector.h \
    engineworker.h \
    ChessEnginePlayer.h

DISTFILES += \
    ../../UciConnector/UciConnector.vcxproj \
    ../../UciConnector/UciConnector.vcxproj.filters \
    Button.qml \
    ChessBoard.qml \
    ChessBoxes.qml \
    ChessFigures.qml \
    ChessGame.qml \
    CustomSlider.qml \
    EmptyBoardModel.qml \
    MarkerColumn.qml \
    MarkerRow.qml \
    NavigationLayer.qml \
    Notificator.qml \
    PawnPromotionOptions.qml \
    NetworkPlayers.qml \
    SelectPlayerDialog.qml
