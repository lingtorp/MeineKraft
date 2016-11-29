# MeineKraft - *Rendering engine written in C++14 & OpenGL*
![Perlin Noise generated terrain](/screenshots/perlin-hills.gif)
![Linear fog](/screenshots/linear-fog.gif)
![Dynamic shader editing, with reloading!](/screenshots/dynamic-shader-editing.gif)
![Basic lighting](/screenshots/moving-lights.gif)

This project serves as a study for me into the realm of computer graphics and other
related areas. Currently I am writing this during my spare time. My intent is to
follow up with some articles relating to my learning experience, these articles
can be found on my personal [site](lingtorp.com).

# Build
All the dependencies are bundled within the project folder /include.
Currently only using header-only libraries.
### *CLion*
1. Download the project
2. Open with CLion, boom!

### *CMake*
Since it is a Cmake based project someone with the skills in Cmake can make this happen!

# Dependencies
* [dear imgui](https://github.com/ocornut/imgui)
* [tinyobjloader](https://github.com/syoyo/tinyobjloader)

# Notebook
* Procedural generation
    * Perlin Noise
    * Simplex Noise'
- [ ] Add all dependices to the README
- [ ] Build instructions ..

Content streaming
- [ ] Load new chunk dynamically depending on Player position
- [ ] Deallocate old chunks
- [ ] Save deallocated chunks (SQLite?)
- [ ] Multithreaded?

Lightning
- [ ] Implement Phong lightning to begin with

Rendering
- [x] Merge all model matrices into a single matrix
- [x] Use instanced rendering
- [x] Line of sight distance
- [x] View frustrum culling (geometric approach)
- [ ] Ray tracing for entity selection
- [ ] Merge all 3 transformation matrices into one
- [ ] Frustrum cull checks against parent Nodes (nodes with children)
- [x] Resolution independence
- [x] Watch shader files for changes (auto) reload them.
- [x] Importing Meshes (tinyobjloader)
- [x] Deduplication of imported Meshes
- [x] Immediate mode GUI (imgui)
- [ ] Use quaterions
- [ ] Scene graph - with transforms and node parents
- [ ]  Hit point cursor
- [ ] Smoother movement
- [ ] Avoid costly matrix multiplications somehow (scale, rotate_XYZ, etc)
- [x] Add atomics to the FileMonitoring
- [ ] Add move constructors and move assignment operators to all Vector types, measure the performance increase!
- [ ] Make Vector types for-range-able
- [x] Import .obj with texture data and more
- [ ] Normal mapping!
- [x] Diffuse textures
- [ ] Console with lua or something to apply translations/transforms
- [ ] Quaternions!
- [x] Multiple lights - uniform buffers
- [ ] Per object diffuse texture - texture buffers (?)
- [ ] Octaves of Perlin noise
- [ ] Translate normals and trnasform them if object moves
- [ ] Implement flat shading
- [ ] !, Gouraud shading
- [ ] Blink-Phong shading
- [ ] Light material: light power, light color,

Voxelize
- [ ] Food, donuts, etc
- [ ] Freja
- [ ] Pokemons!
- [ ] Chips, ICs!
- [ ] Macbooks
- [ ] Handgranat

Code hygiene
- [ ] Prefix all functions that touch OpenGL with "gl_"
- [ ] Change all OpenGL specific location from uint64_t to uint32_t since OpenGL maps it's indices to int (32 bits).

# License
The MIT License (MIT)
Copyright (c) 2016 Alexander Lingtorp

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
