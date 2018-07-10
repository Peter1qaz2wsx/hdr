cmake_minimum_required(VERSION 2.8)

project(mai11_tone_mapping)
#ls2
find_package(OpenCV REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -O3")
add_definitions(-std=c++11)

add_library(${PROJECT_NAME} SHARED
lib/compression_tmo.cpp
lib/progress.cpp
lib/Autolevels.cpp
lib/gamma_levels.cpp
)


add_executable(tone_mapping
main.cpp
)
include_directories(
${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(tone_mapping
${OpenCV_LIBS}
)

target_link_libraries(tone_mapping ${PROJECT_NAME})
