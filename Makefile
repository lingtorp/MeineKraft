# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = "/Users/lingtorp/Library/Application Support/JetBrains/Toolbox/apps/CLion/ch-0/182.3684.76/CLion.app/Contents/bin/cmake/mac/bin/cmake"

# The command to remove a file.
RM = "/Users/lingtorp/Library/Application Support/JetBrains/Toolbox/apps/CLion/ch-0/182.3684.76/CLion.app/Contents/bin/cmake/mac/bin/cmake" -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/lingtorp/repos/MeineKraft

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/lingtorp/repos/MeineKraft

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	"/Users/lingtorp/Library/Application Support/JetBrains/Toolbox/apps/CLion/ch-0/182.3684.76/CLion.app/Contents/bin/cmake/mac/bin/cmake" -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	"/Users/lingtorp/Library/Application Support/JetBrains/Toolbox/apps/CLion/ch-0/182.3684.76/CLion.app/Contents/bin/cmake/mac/bin/cmake" -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/lingtorp/repos/MeineKraft/CMakeFiles /Users/lingtorp/repos/MeineKraft/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/lingtorp/repos/MeineKraft/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named MeineKraft

# Build rule for target.
MeineKraft: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 MeineKraft
.PHONY : MeineKraft

# fast build rule for target.
MeineKraft/fast:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/build
.PHONY : MeineKraft/fast

include/imgui/imgui.o: include/imgui/imgui.cpp.o

.PHONY : include/imgui/imgui.o

# target to build an object file
include/imgui/imgui.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/include/imgui/imgui.cpp.o
.PHONY : include/imgui/imgui.cpp.o

include/imgui/imgui.i: include/imgui/imgui.cpp.i

.PHONY : include/imgui/imgui.i

# target to preprocess a source file
include/imgui/imgui.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/include/imgui/imgui.cpp.i
.PHONY : include/imgui/imgui.cpp.i

include/imgui/imgui.s: include/imgui/imgui.cpp.s

.PHONY : include/imgui/imgui.s

# target to generate assembly for a file
include/imgui/imgui.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/include/imgui/imgui.cpp.s
.PHONY : include/imgui/imgui.cpp.s

include/imgui/imgui_demo.o: include/imgui/imgui_demo.cpp.o

.PHONY : include/imgui/imgui_demo.o

# target to build an object file
include/imgui/imgui_demo.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/include/imgui/imgui_demo.cpp.o
.PHONY : include/imgui/imgui_demo.cpp.o

include/imgui/imgui_demo.i: include/imgui/imgui_demo.cpp.i

.PHONY : include/imgui/imgui_demo.i

# target to preprocess a source file
include/imgui/imgui_demo.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/include/imgui/imgui_demo.cpp.i
.PHONY : include/imgui/imgui_demo.cpp.i

include/imgui/imgui_demo.s: include/imgui/imgui_demo.cpp.s

.PHONY : include/imgui/imgui_demo.s

# target to generate assembly for a file
include/imgui/imgui_demo.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/include/imgui/imgui_demo.cpp.s
.PHONY : include/imgui/imgui_demo.cpp.s

include/imgui/imgui_draw.o: include/imgui/imgui_draw.cpp.o

.PHONY : include/imgui/imgui_draw.o

# target to build an object file
include/imgui/imgui_draw.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/include/imgui/imgui_draw.cpp.o
.PHONY : include/imgui/imgui_draw.cpp.o

include/imgui/imgui_draw.i: include/imgui/imgui_draw.cpp.i

.PHONY : include/imgui/imgui_draw.i

# target to preprocess a source file
include/imgui/imgui_draw.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/include/imgui/imgui_draw.cpp.i
.PHONY : include/imgui/imgui_draw.cpp.i

include/imgui/imgui_draw.s: include/imgui/imgui_draw.cpp.s

.PHONY : include/imgui/imgui_draw.s

# target to generate assembly for a file
include/imgui/imgui_draw.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/include/imgui/imgui_draw.cpp.s
.PHONY : include/imgui/imgui_draw.cpp.s

main.o: main.cpp.o

.PHONY : main.o

# target to build an object file
main.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/main.cpp.o
.PHONY : main.cpp.o

main.i: main.cpp.i

.PHONY : main.i

# target to preprocess a source file
main.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/main.cpp.i
.PHONY : main.cpp.i

main.s: main.cpp.s

.PHONY : main.s

# target to generate assembly for a file
main.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/main.cpp.s
.PHONY : main.cpp.s

nodes/entity.o: nodes/entity.cpp.o

.PHONY : nodes/entity.o

# target to build an object file
nodes/entity.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/nodes/entity.cpp.o
.PHONY : nodes/entity.cpp.o

nodes/entity.i: nodes/entity.cpp.i

.PHONY : nodes/entity.i

# target to preprocess a source file
nodes/entity.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/nodes/entity.cpp.i
.PHONY : nodes/entity.cpp.i

nodes/entity.s: nodes/entity.cpp.s

.PHONY : nodes/entity.s

# target to generate assembly for a file
nodes/entity.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/nodes/entity.cpp.s
.PHONY : nodes/entity.cpp.s

nodes/model.o: nodes/model.cpp.o

.PHONY : nodes/model.o

# target to build an object file
nodes/model.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/nodes/model.cpp.o
.PHONY : nodes/model.cpp.o

nodes/model.i: nodes/model.cpp.i

.PHONY : nodes/model.i

# target to preprocess a source file
nodes/model.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/nodes/model.cpp.i
.PHONY : nodes/model.cpp.i

nodes/model.s: nodes/model.cpp.s

.PHONY : nodes/model.s

# target to generate assembly for a file
nodes/model.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/nodes/model.cpp.s
.PHONY : nodes/model.cpp.s

nodes/skybox.o: nodes/skybox.cpp.o

.PHONY : nodes/skybox.o

# target to build an object file
nodes/skybox.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/nodes/skybox.cpp.o
.PHONY : nodes/skybox.cpp.o

nodes/skybox.i: nodes/skybox.cpp.i

.PHONY : nodes/skybox.i

# target to preprocess a source file
nodes/skybox.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/nodes/skybox.cpp.i
.PHONY : nodes/skybox.cpp.i

nodes/skybox.s: nodes/skybox.cpp.s

.PHONY : nodes/skybox.s

# target to generate assembly for a file
nodes/skybox.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/nodes/skybox.cpp.s
.PHONY : nodes/skybox.cpp.s

render/camera.o: render/camera.cpp.o

.PHONY : render/camera.o

# target to build an object file
render/camera.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/camera.cpp.o
.PHONY : render/camera.cpp.o

render/camera.i: render/camera.cpp.i

.PHONY : render/camera.i

# target to preprocess a source file
render/camera.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/camera.cpp.i
.PHONY : render/camera.cpp.i

render/camera.s: render/camera.cpp.s

.PHONY : render/camera.s

# target to generate assembly for a file
render/camera.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/camera.cpp.s
.PHONY : render/camera.cpp.s

render/meshmanager.o: render/meshmanager.cpp.o

.PHONY : render/meshmanager.o

# target to build an object file
render/meshmanager.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/meshmanager.cpp.o
.PHONY : render/meshmanager.cpp.o

render/meshmanager.i: render/meshmanager.cpp.i

.PHONY : render/meshmanager.i

# target to preprocess a source file
render/meshmanager.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/meshmanager.cpp.i
.PHONY : render/meshmanager.cpp.i

render/meshmanager.s: render/meshmanager.cpp.s

.PHONY : render/meshmanager.s

# target to generate assembly for a file
render/meshmanager.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/meshmanager.cpp.s
.PHONY : render/meshmanager.cpp.s

render/render.o: render/render.cpp.o

.PHONY : render/render.o

# target to build an object file
render/render.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/render.cpp.o
.PHONY : render/render.cpp.o

render/render.i: render/render.cpp.i

.PHONY : render/render.i

# target to preprocess a source file
render/render.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/render.cpp.i
.PHONY : render/render.cpp.i

render/render.s: render/render.cpp.s

.PHONY : render/render.s

# target to generate assembly for a file
render/render.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/render.cpp.s
.PHONY : render/render.cpp.s

render/rendercomponent.o: render/rendercomponent.cpp.o

.PHONY : render/rendercomponent.o

# target to build an object file
render/rendercomponent.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/rendercomponent.cpp.o
.PHONY : render/rendercomponent.cpp.o

render/rendercomponent.i: render/rendercomponent.cpp.i

.PHONY : render/rendercomponent.i

# target to preprocess a source file
render/rendercomponent.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/rendercomponent.cpp.i
.PHONY : render/rendercomponent.cpp.i

render/rendercomponent.s: render/rendercomponent.cpp.s

.PHONY : render/rendercomponent.s

# target to generate assembly for a file
render/rendercomponent.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/rendercomponent.cpp.s
.PHONY : render/rendercomponent.cpp.s

render/shader.o: render/shader.cpp.o

.PHONY : render/shader.o

# target to build an object file
render/shader.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/shader.cpp.o
.PHONY : render/shader.cpp.o

render/shader.i: render/shader.cpp.i

.PHONY : render/shader.i

# target to preprocess a source file
render/shader.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/shader.cpp.i
.PHONY : render/shader.cpp.i

render/shader.s: render/shader.cpp.s

.PHONY : render/shader.s

# target to generate assembly for a file
render/shader.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/shader.cpp.s
.PHONY : render/shader.cpp.s

render/texture.o: render/texture.cpp.o

.PHONY : render/texture.o

# target to build an object file
render/texture.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/texture.cpp.o
.PHONY : render/texture.cpp.o

render/texture.i: render/texture.cpp.i

.PHONY : render/texture.i

# target to preprocess a source file
render/texture.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/texture.cpp.i
.PHONY : render/texture.cpp.i

render/texture.s: render/texture.cpp.s

.PHONY : render/texture.s

# target to generate assembly for a file
render/texture.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/render/texture.cpp.s
.PHONY : render/texture.cpp.s

scene/world.o: scene/world.cpp.o

.PHONY : scene/world.o

# target to build an object file
scene/world.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/scene/world.cpp.o
.PHONY : scene/world.cpp.o

scene/world.i: scene/world.cpp.i

.PHONY : scene/world.i

# target to preprocess a source file
scene/world.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/scene/world.cpp.i
.PHONY : scene/world.cpp.i

scene/world.s: scene/world.cpp.s

.PHONY : scene/world.s

# target to generate assembly for a file
scene/world.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/scene/world.cpp.s
.PHONY : scene/world.cpp.s

util/filemonitor.o: util/filemonitor.cpp.o

.PHONY : util/filemonitor.o

# target to build an object file
util/filemonitor.cpp.o:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/util/filemonitor.cpp.o
.PHONY : util/filemonitor.cpp.o

util/filemonitor.i: util/filemonitor.cpp.i

.PHONY : util/filemonitor.i

# target to preprocess a source file
util/filemonitor.cpp.i:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/util/filemonitor.cpp.i
.PHONY : util/filemonitor.cpp.i

util/filemonitor.s: util/filemonitor.cpp.s

.PHONY : util/filemonitor.s

# target to generate assembly for a file
util/filemonitor.cpp.s:
	$(MAKE) -f CMakeFiles/MeineKraft.dir/build.make CMakeFiles/MeineKraft.dir/util/filemonitor.cpp.s
.PHONY : util/filemonitor.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... rebuild_cache"
	@echo "... edit_cache"
	@echo "... MeineKraft"
	@echo "... include/imgui/imgui.o"
	@echo "... include/imgui/imgui.i"
	@echo "... include/imgui/imgui.s"
	@echo "... include/imgui/imgui_demo.o"
	@echo "... include/imgui/imgui_demo.i"
	@echo "... include/imgui/imgui_demo.s"
	@echo "... include/imgui/imgui_draw.o"
	@echo "... include/imgui/imgui_draw.i"
	@echo "... include/imgui/imgui_draw.s"
	@echo "... main.o"
	@echo "... main.i"
	@echo "... main.s"
	@echo "... nodes/entity.o"
	@echo "... nodes/entity.i"
	@echo "... nodes/entity.s"
	@echo "... nodes/model.o"
	@echo "... nodes/model.i"
	@echo "... nodes/model.s"
	@echo "... nodes/skybox.o"
	@echo "... nodes/skybox.i"
	@echo "... nodes/skybox.s"
	@echo "... render/camera.o"
	@echo "... render/camera.i"
	@echo "... render/camera.s"
	@echo "... render/meshmanager.o"
	@echo "... render/meshmanager.i"
	@echo "... render/meshmanager.s"
	@echo "... render/render.o"
	@echo "... render/render.i"
	@echo "... render/render.s"
	@echo "... render/rendercomponent.o"
	@echo "... render/rendercomponent.i"
	@echo "... render/rendercomponent.s"
	@echo "... render/shader.o"
	@echo "... render/shader.i"
	@echo "... render/shader.s"
	@echo "... render/texture.o"
	@echo "... render/texture.i"
	@echo "... render/texture.s"
	@echo "... scene/world.o"
	@echo "... scene/world.i"
	@echo "... scene/world.s"
	@echo "... util/filemonitor.o"
	@echo "... util/filemonitor.i"
	@echo "... util/filemonitor.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

