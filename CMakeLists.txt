cmake_minimum_required(VERSION 3.5)
project(MeineKraft)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp render/render.cpp render/render.h math/vector.h world/world.cpp world/world.h render/primitives.h render/camera.cpp render/camera.h render/chunk.cpp render/chunk.h)
add_executable(MeineKraft ${SOURCE_FILES})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
target_link_libraries(MeineKraft ${GLEW_LIBRARIES})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(MeineKraft ${OPENGL_LIBRARIES})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(MeineKraft ${SDL2_LIBRARIES})

cmake_policy(SET CMP0004 OLD)
include(FindPkgConfig)
pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
include_directories({SDL2IMAGE_INCLUDE_DIRS})
target_link_libraries(MeineKraft ${SDL2IMAGE_LIBRARIES})