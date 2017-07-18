cmake_minimum_required(VERSION 3.5)
project(MeineKraft)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread") # pthread for Linux
endif(UNIX)
set(CMAKE_CXX_STANDARD 14)

file(GLOB IMGUI_SRC "include/imgui/*.cpp" "include/imgui/*.h") # Adds all imgui related files into the application

set(SOURCE_FILES main.cpp render/render.cpp render/render.h math/vector.h world/world.cpp world/world.h render/primitives.h
        render/camera.cpp render/camera.h nodes/chunk.cpp nodes/chunk.h math/noise.h render/ray.h render/graphicsbatch.h
        render/graphicsbatch.cpp render/rendercomponent.cpp render/rendercomponent.h nodes/entity.cpp nodes/entity.h
        render/shader.cpp render/shader.h ${IMGUI_SRC} nodes/skybox.cpp nodes/skybox.h nodes/block.cpp nodes/block.h
        nodes/skybox.cpp nodes/skybox.h ${TINYOBJLOADER_SRC} nodes/teapot.cpp nodes/teapot.h util/filemonitor.cpp
        util/filemonitor.h render/texture.cpp render/texture.h render/transform.h render/light.h render/meshmanager.cpp
        render/meshmanager.h render/texturemanager.cpp render/texturemanager.h)
add_executable(MeineKraft ${SOURCE_FILES})

find_package(ASSIMP REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIRS})
target_link_libraries(MeineKraft ${ASSIMP_LIBRARIES})

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

FIND_PATH(SDL2IMAGE_INCLUDE_DIR SDL_image.h
        HINTS
        $ENV{SDL2IMAGEDIR}
        $ENV{SDL2DIR}
        PATH_SUFFIXES include
        PATHS
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/include/SDL2
        /usr/include/SDL2
        /sw/include/SDL2 # Fink
        /opt/local/include/SDL2 # DarwinPorts
        /opt/csw/include/SDL2 # Blastwave
        /opt/include/SDL2
        )

FIND_LIBRARY(SDL2IMAGE_LIBRARY
        NAMES SDL2_image
        HINTS
        $ENV{SDL2IMAGEDIR}
        $ENV{SDL2DIR}
        PATH_SUFFIXES lib64 lib
        PATHS
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt
        )

include_directories({SDL2IMAGE_INCLUDE_DIR})
target_link_libraries(MeineKraft ${SDL2IMAGE_LIBRARY})