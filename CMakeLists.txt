cmake_minimum_required(VERSION 3.0.0)

project(CS3205_Assignment2_TicTacToe)

find_package(Threads REQUIRED)

include_directories(include)

add_library(tictactoe STATIC Tictactoe/tictactoe.cpp Tictactoe/logger.cpp)
add_executable(gserver Game/gameserver.cpp)
add_executable(gclient Game/gameclient.cpp)

target_link_libraries(gserver ${CMAKE_THREAD_LIBS_INIT} tictactoe)