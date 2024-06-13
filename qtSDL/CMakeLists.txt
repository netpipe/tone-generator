cmake_minimum_required(VERSION 3.5)

project(ToneGenerator)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS})

add_executable(ToneGenerator main.cpp)

target_link_libraries(ToneGenerator Qt5::Widgets ${SDL2_LIBRARIES} ${SDL2_MIXER_LIBRARIES})
