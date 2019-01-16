# MeineKraft

[![MIT license](https://img.shields.io/badge/License-MIT-blue.svg)](https://lbesson.mit-license.org/)
[![](https://img.shields.io/badge/twitter-follow-blue.svg)](https://twitter.com/ALingtorp)

![](/screenshots/pbr-1.png)

**MeineKraft** is a **physically based rendering engine** written in **C++11** & **OpenGL 4.6**.
My intent is to follow up the implementation of the most interesting parts with some articles relating to my learning experience,
these articles can be found on my personal [site](http://lingtorp.com) with the tag *MeineKraft*.

| Various | GIFs |
| ------------- | ------------- |
| Perlin noise ![Perlin Noise generated terrain](/screenshots/perlin-hills.gif) | Linear fog![Linear fog](/screenshots/linear-fog.gif) |
| Dynamic shader reloading![Dynamic shader editing, with reloading!](/screenshots/dynamic-shader-editing.gif) | Phong reflection model ![Basic lighting](/screenshots/moving-lights.gif) |

It does include some game engine related tech as well such as a Entity-Component-System
architecture. The main game object in the engine is a object-oriented layer
on top of the ECS architecture in order to make it slightly easier to write
gameplay code while keeping the performance and data-oriented architecture intact.

## Dependencies
All of the dependencies are bundled within the folder /include, /bin, /lib.
* [dear imgui](https://github.com/ocornut/imgui) - debug GUI.
* [assimp](https://github.com/syoyo/assimp) - model importing.
* [SDL2](https://www.libsdl.org/) - window creation
* [SDL-image](https://www.libsdl.org/projects/SDL_image/) - image loading and conversion
* [GLEW](https://duckduckgo.com/?q=GLEW&t=ffab&ia=web) - OpenGL function loader

Platforms supported: Windows and Linux, macOS support is not possible due to
unsupported OpenGL version.

## Build
Before continueing go to the file filesystem.h and adjust the filepaths so that they match your system.
### Linux (Ubuntu)
```bash
sudo apt install libsdl2-dev libsdl2-image-dev libassimp-dev libglew-dev
cmake --build .
cmake .
./MeineKraft
```
### Windows
Launch CMake-GUI and select repository root directory and then the build directory.
Generate the Visual Studio 2017 solution and simply build it via Visual Studio.
### macOS
Since OpenGL is deprecated on macOS this will probably not work in the future.
```bash
sudo brew install glew sdl2 assimp
cmake --build .
cmake . 
./MeineKraft
```

# License
The MIT License (MIT)
Copyright (c) 2017 Alexander Lingtorp

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
