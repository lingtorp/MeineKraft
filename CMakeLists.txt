cmake_minimum_required(VERSION 3.10)
project(MeineKraft)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread") # pthread for Linux
endif(UNIX)

set(CMAKE_CXX_STANDARD 11)

if(WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4") # Enable warning level 4
endif(WIN32)

file(GLOB IMGUI_SRC "include/imgui/*.cpp" "include/imgui/*.h") # Adds all imgui related files into the application

include_directories(include)
include_directories(include/assimp)
include_directories(include/glew)
include_directories(include/sdl2)
include_directories(include/sdl2-image)

set(MATH_SRC_FILES "math/noise.h" "math/vector.h" "math/quaternion.h")
source_group("math" FILES ${MATH_SRC_FILES})

set(NODES_SRC_FILES "nodes/transform.h" "nodes/skybox.cpp" "nodes/skybox.h" "nodes/model.cpp" "nodes/model.h" "nodes/entity.cpp" "nodes/entity.h")
source_group("nodes" FILES ${NODES_SRC_FILES})

set(RENDER_SRC_FILES "render/shader.cpp" "render/shader.h" "render/texture.cpp" "render/texture.h" 
        "render/rendercomponent.cpp" "render/rendercomponent.h" "render/ray.h" "render/graphicsbatch.h"
        "render/render.cpp" "render/render.h" "render/primitives.h"
        "render/camera.cpp" "render/camera.h" "render/debug_opengl.h"
        "render/light.h" "render/meshmanager.cpp" "render/meshmanager.h" "render/texturemanager.h")
source_group("render" FILES ${RENDER_SRC_FILES})

set(UTIL_SRC_FILES "util/filemonitor.cpp" "util/filemonitor.h" "util/filesystem.h" "util/stb_image.h" "util/logging.h")
source_group("util" FILES ${UTIL_SRC_FILES})

set(SCENE_SRC_FILES "scene/world.cpp" "scene/world.hpp")
source_group("scene" FILES ${SCENE_SRC_FILES})

set(SHADER_SRC_FILES "shaders/blur-fragment.glsl" "shaders/blur-vertex.glsl" "shaders/geometry-fragment.glsl"
        "shaders/geometry-vertex.glsl" "shaders/lightning-fragment.glsl" "shaders/lightning-vertex.glsl"
        "shaders/ssao-fragment.glsl" "shaders/ssao-vertex.glsl" "shaders/culling.comp.glsl")
source_group("shaders" FILES ${SHADER_SRC_FILES})

set(SOURCE_FILES main.cpp ${MATH_SRC_FILES} ${NODES_SRC_FILES} ${RENDER_SRC_FILES} ${UTIL_SRC_FILES} ${SCENE_SRC_FILES} ${IMGUI_SRC})
add_executable(MeineKraft ${SOURCE_FILES})

if(WIN32)
        # Turn on using solution folders for VS
        set_property(GLOBAL PROPERTY USE_FOLDERS ON)

        # Dumps the .DLLs at the same place as the .exe depending on the build type 
        add_custom_command(TARGET MeineKraft POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/bin
                ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE})
endif(WIN32)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(MeineKraft ${OPENGL_LIBRARIES})

if(WIN32)
        find_library(ASSIMP_LIBRARIES NAMES assimp assimp.dll PATHS ${CMAKE_SOURCE_DIR}/libs)
        target_link_libraries(MeineKraft ${ASSIMP_LIBRARIES})

        find_library(SDL2_LIBRARIES NAMES SDL2 SDL2.dll PATHS ${CMAKE_SOURCE_DIR}/libs)
        target_link_libraries(MeineKraft ${SDL2_LIBRARIES})

        find_library(GLEW_LIBRARIES NAMES glew32 glew32.dll PATHS ${CMAKE_SOURCE_DIR}/libs)
        target_link_libraries(MeineKraft ${GLEW_LIBRARIES})

        find_library(SDL2IMAGE_LIBRARIES NAMES sdl2_image SDL2_image.dll PATHS ${CMAKE_SOURCE_DIR}/libs)
        target_link_libraries(MeineKraft ${SDL2IMAGE_LIBRARIES})
else(WIN32)
        find_package(ASSIMP REQUIRED)
        include_directories(${ASSIMP_INCLUDE_DIRS})
        target_link_libraries(MeineKraft ${ASSIMP_LIBRARIES})

        find_package(SDL2 REQUIRED)
        include_directories(${SDL2_INCLUDE_DIRS})
        target_link_libraries(MeineKraft ${SDL2_LIBRARIES})

        find_package(GLEW REQUIRED)
        include_directories(${GLEW_INCLUDE_DIRS})
        target_link_libraries(MeineKraft ${GLEW_LIBRARIES})

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
endif(WIN32)