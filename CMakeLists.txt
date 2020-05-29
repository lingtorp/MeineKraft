cmake_minimum_required(VERSION 3.13)
project(MeineKraft)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread") # pthread for Linux
endif(UNIX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# MeineKraft build variables
set(MEINEKRAFT_VAR_RENDERER VK) # GL = OpenGL, VK = Vulkan

if(WIN32)
        # Get rid of annoying MSVC warnings.
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)

        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /d2cgsummary") #
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Bt") #  time spent in frontend (c1xx.dll) vs backend (c2.dll)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4") # Enable warning level 4
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # MultiProcessor compilation
endif(WIN32)

file(GLOB IMGUI_SRC "include/imgui/*.cpp" "include/imgui/*.h") # Adds all imgui related files into the application

include_directories(include)
include_directories(include/assimp)
include_directories(include/glew)
include_directories(include/sdl2)
include_directories(include/sdl2-image)
include_directories(include/json)

set(MATH_SRC_FILES "src/math/noise.hpp" "src/math/vector.hpp" "src/math/quaternion.hpp")
source_group("math" FILES ${MATH_SRC_FILES})

set(NODES_SRC_FILES "src/nodes/transform.hpp" "src/nodes/skybox.cpp" "src/nodes/skybox.hpp" "src/nodes/model.cpp" "src/nodes/model.hpp" "src/nodes/entity.cpp" "src/nodes/entity.hpp")
source_group("nodes" FILES ${NODES_SRC_FILES})

set(RENDER_SRC_FILES "src/rendering/shader.cpp" "src/rendering/shader.hpp"
        "src/rendering/texture.cpp" "src/rendering/texture.hpp"
        "src/rendering/rendercomponent.cpp" "src/rendering/rendercomponent.hpp"
        "src/rendering/ray.hpp" "src/rendering/graphicsbatch.hpp"
        "src/rendering/render_system.hpp" "src/rendering/render_system.cpp"
        "src/rendering/vk/vk_renderer.cpp" "src/rendering/vk/vk_renderer.hpp"
        "src/rendering/vk/vk_debug.hpp"
        "src/rendering/primitives.hpp" "src/rendering/camera.cpp"
        "src/rendering/camera.hpp"
        "src/rendering/light.hpp" "src/rendering/meshmanager.cpp"
        "src/rendering/meshmanager.hpp" "src/rendering/texturemanager.hpp"
        "src/rendering/renderpass/renderpass.hpp"
        "src/rendering/renderpass/renderpass.cpp")
source_group("render" FILES ${RENDER_SRC_FILES})

set(UTIL_SRC_FILES "src/util/filemonitor.cpp" "src/util/filemonitor.hpp" "src/util/filesystem.hpp" "src/util/logging.hpp" "src/util/logging.cpp" "src/util/config.hpp" "src/util/config.cpp" "src/util/logging_system.hpp" "src/util/logging_system.cpp" "src/util/mkass.cpp" "src/util/mkass.hpp")
source_group("util" FILES ${UTIL_SRC_FILES})

set(SCENE_SRC_FILES "src/scene/world.cpp" "src/scene/world.hpp")
source_group("scene" FILES ${SCENE_SRC_FILES})

# NOTE: Not needed for compilation but its nice to have the shaders visible in Visual Studio
set(SHADER_SRC_FILES "shaders/blur.vert" "shaders/blur.frag"
        "shaders/geometry.vert" "shaders/geometry.frag" "shaders/lightning.vert"
        "shaders/lightning.frag" "shaders/ssao.frag" "shaders/ssao.vert"
        "shaders/culling.comp.glsl" "shaders/voxel-cone-tracing.vert"
        "shaders/voxel-cone-tracing.frag" "shaders/voxelization.vert"
        "shaders/voxelization.geom" "shaders/voxelization.frag"
        "shaders/voxelization-opacity-normalization.comp")
source_group("shaders" FILES ${SHADER_SRC_FILES})

set(SOURCE_FILES main.cpp src/meinekraft.hpp src/meinekraft.cpp ${MATH_SRC_FILES} ${NODES_SRC_FILES} ${RENDER_SRC_FILES} ${UTIL_SRC_FILES} ${SCENE_SRC_FILES} ${IMGUI_SRC} ${SHADER_SRC_FILES})
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

if(APPLE)
  find_library(MoltenVK NAMES "libMoltenVK.dylib" HINTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/MoltenVK/macOS")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework Metal -framework IOKit -framework CoreVideo")
  include_directories("${PROJECT_SOURCE_DIR}/ext/MoltenVK/include")
else(APPLE)
  find_package(Vulkan REQUIRED)
  include_directories(${Vulkan_INCLUDE_DIRS})
  target_link_libraries(MeineKraft ${Vulkan_LIBRARIES})
endif(APPLE)

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
        find_package(ASSIMP 5.0.0 REQUIRED)
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
