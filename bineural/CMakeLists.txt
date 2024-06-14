cmake_minimum_required(VERSION 3.10)

project(ToneGenerator)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Charts REQUIRED)

include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5Charts_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS} ${Qt5Charts_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} ${Qt5Charts_EXECUTABLE_COMPILE_FLAGS}")

add_executable(ToneGenerator main.cpp)

target_link_libraries(ToneGenerator Qt5::Widgets Qt5::Charts openal)
