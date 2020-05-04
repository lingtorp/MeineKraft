# :exclamation: MeineKraft :exclamation:

[![MIT license](https://img.shields.io/badge/License-MIT-blue.svg)](https://lbesson.mit-license.org/)
[![](https://img.shields.io/badge/twitter-follow-blue.svg)](https://twitter.com/ALingtorp)

![](/resources/screenshots/showcase.png)

**MeineKraft** is a **physically based rendering engine** written in **C++17** & **OpenGL 4.6**.

MeineKraft is built on a core Entity-Component-System architecture. Gameobjects
in the engine is a lighweight object-oriented layer on top of the ECS core
systems in order to make it slightly easier to write gameplay code while keeping
the performance and data-oriented architecture intact.

:memo: Blogposts on the implementation of the most interesting parts can be
found [here](http://lingtorp.com) with the tag *MeineKraft*.

## Features :trophy:
- [X] Voxel cone tracing based global illumination
- - [X] 3D clipmap 
- - [X] joing bilateral upsampling/filtering
- - [X] isotropic voxels
- - [X] emissive materials
- - [X] (_optional_) opacity normalization subpass 
- - [X] second-depth shadow mapping
- - [X] percentage-closer filtering shadow mapping 
- - [X] normal mapping
- [X] Physically based BRDF
- [X] glTF roughness, metallic material model
- [X] Entity-Component-System architecture
- [X] JSON-based configuration
- [X] Editor

## Documentation :memo:
Relevant documentation for each part of the engine is located in the /documentation folder. 

## Dependencies :floppy_disk:
* [dear imgui](https://github.com/ocornut/imgui) - editor GUI
* [assimp](https://github.com/syoyo/assimp) - model/scene importing
* [SDL2](https://www.libsdl.org/) - window creation, input handling
* [SDL-image](https://www.libsdl.org/projects/SDL_image/) - image loading and conversion
* [GLEW](https://duckduckgo.com/?q=GLEW&t=ffab&ia=web) - OpenGL function loader
* [GLM](https://glm.g-truc.net/0.9.8/index.html) - various mathmatical utilities

Platforms supported: Windows and Linux

## Build :computer:
**NOTE:** Before continueing go to the file util/filesystem.hpp and adjust the filepaths so that they match your system.
### Linux (Ubuntu 18.10)
```bash
sudo apt install libsdl2-dev libsdl2-image-dev libassimp-dev libglew-dev
cmake --build .
cmake .
./MeineKraft
```
### Windows
#### Install [Vcpkg](https://github.com/microsoft/vcpkg)
Clone Vcpkg, build it, then run.
```bash
./vcpkg install sdl2:x64-windows sdl2-image:x64-windows glew:x64-windows assimp:x64-windows
```
Launch CMake-GUI and select MeineKraft repository and then create a build directory and select it.
Generate the Visual Studio 2017/2019 solution and simply build and run it via Visual Studio.

# License :books:
The MIT License (MIT)
Copyright (c) 2017 Alexander Lingtorp

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
