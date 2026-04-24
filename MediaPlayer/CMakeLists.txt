cmake_minimum_required(VERSION 3.10)
project(MediaPlayer C)

include_directories(include)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_library(SDL2_TTF_LIBRARY SDL2_ttf)

add_executable(MediaPlayer src/MediaPlayer.c src/tinyfiledialogs.c)

target_link_libraries(MediaPlayer m ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARY})

