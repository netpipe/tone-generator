cmake_minimum_required(VERSION 3.5)

project(ToneGenerator)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(OpenAL REQUIRED)

include_directories(${OPENAL_INCLUDE_DIR})

add_executable(ToneGenerator main.cpp)

target_link_libraries(ToneGenerator Qt5::Widgets ${OPENAL_LIBRARY})
