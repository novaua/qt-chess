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
    ChessEnginePlayer.cpp \
    AppInfo.cpp \
    AvatarProvider.cpp \
    UserManager.cpp

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
    ChessEnginePlayer.h \
    AppInfo.h \
    AvatarProvider.h \
    UserManager.h

DISTFILES += \
    qml/AppDialog.qml \
    qml/ApplyMovesDialog.qml \
    qml/CapturedStrip.qml \
    qml/ChallengeDialog.qml \
    qml/ChessBoard.qml \
    qml/ChessBoxes.qml \
    qml/ChessFigures.qml \
    qml/ChessGame.qml \
    qml/ChoiceRow.qml \
    qml/CommandBar.qml \
    qml/ComputerGameDialog.qml \
    qml/ConfirmDialog.qml \
    qml/CreateUserDialog.qml \
    qml/EmptyBoardModel.qml \
    qml/GamePage.qml \
    qml/HomeCard.qml \
    qml/FocusRing.qml \
    qml/HomePage.qml \
    qml/IconButton.qml \
    qml/InfoBar.qml \
    qml/LevelsHelpDialog.qml \
    qml/LichessGameDialog.qml \
    qml/MarkerColumn.qml \
    qml/MarkerRow.qml \
    qml/MoveList.qml \
    qml/MovesPanel.qml \
    qml/NavigationLayer.qml \
    qml/PawnPromotionOptions.qml \
    qml/PlayerCard.qml \
    qml/ResultCard.qml \
    qml/AppSettings.qml \
    qml/RestoreBackupDialog.qml \
    qml/SettingRow.qml \
    qml/SettingsPage.qml \
    qml/Theme.qml \
    qml/TwoPlayerDialog.qml \
    qml/WelcomePage.qml \
    qml/ProfilePickerPage.qml \
    qml/ProfilePage.qml \
    qml/AvatarPicker.qml \
    qml/ScreenshotScene.qml \
    qml/SectionCard.qml \
    qml/ImportConflictDialog.qml \
    qml/qmldir
