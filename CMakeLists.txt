cmake_minimum_required(VERSION 3.16)

project(auto_chess1_0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Disable std::filesystem usage in Qt
add_definitions(-DQT_NO_FILESYSTEM)

# Add compiler flags to disable std::filesystem
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_FILESYSTEM")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(auto_chess1_0
    advanceditems.cpp
    board.cpp
    gamemanager.cpp
    main.cpp
    mainwindow.cpp
    player.cpp
    rightcontrolpanel.cpp
    unit.cpp
    advanceditems.h
    board.h
    gamemanager.h
    mainwindow.h
    player.h
    rightcontrolpanel.h
    unit.h
    Item.h
    heroes.h
    mainwindow.ui
)

target_link_libraries(auto_chess1_0 PRIVATE Qt6::Widgets)
