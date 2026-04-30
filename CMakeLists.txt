cmake_minimum_required(VERSION 3.20)
project(ChessGui VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Widgets QuickWidgets Network)

set(SOURCES
    ChessCore/Board.cpp
    ChessCore/BoardPositionsCache.cpp
    ChessCore/Check.cpp
    ChessCore/ChessException.cpp
    ChessCore/Events.cpp
    ChessCore/Game.cpp
    ChessCore/HistoryPlayer.cpp
    ChessCore/Move.cpp
    ChessCore/Piece.cpp
    ChessCore/stdafx.cpp
    UciConnector/UciConnector.cpp
    UciConnector/pch.cpp
    ChessGui/ChessGui/main.cpp
    ChessGui/ChessGui/chessconnector.cpp
    ChessGui/ChessGui/ChessEnginePlayer.cpp
    ChessGui/ChessGui/networkplayer.cpp
    ChessGui/ChessGui/qml.qrc
)

qt_add_executable(ChessGui WIN32 ${SOURCES})

target_include_directories(ChessGui PRIVATE
    ChessCore
    UciConnector
    ChessGui/ChessGui
)

target_link_libraries(ChessGui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets
    Qt6::QuickWidgets
    Qt6::Network
)
