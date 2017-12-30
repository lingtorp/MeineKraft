# MeineKraft

| Various | GIFs |
| ------------- | ------------- |
| Perlin noise ![Perlin Noise generated terrain](/screenshots/perlin-hills.gif) | Linear fog![Linear fog](/screenshots/linear-fog.gif) |
| Dynamic shader reloading![Dynamic shader editing, with reloading!](/screenshots/dynamic-shader-editing.gif) | Phong reflection model ![Basic lighting](/screenshots/moving-lights.gif) |


**MeineKraft** is a **rendering engine** written in **C++14** & **OpenGL 4.1**.
My intent is to follow up the implementation of the most interesting parts with some articles relating to my learning experience,
these articles can be found on my personal [site](http://lingtorp.com).

# Build
All the dependencies are bundled within the folder /include.

# Dependencies
* [dear imgui](https://github.com/ocornut/imgui) for debug GUI.
* [assimp](https://github.com/syoyo/assimp) for model importing.

# TODO
- [ ] Procedural generation
    - [ ] Perlin Noise
    - [ ] Simplex Noise

Lightning
- [x] Phong lightning 

Rendering
- [x] Merge all model matrices into a single matrix
- [x] Use instanced rendering
- [x] Line of sight distance
- [x] View frustrum culling (geometric approach)
- [ ] Ray tracing for entity selection
- [ ] Merge all 3 transformation matrices into one
- [ ] Frustrum cull checks against parent Nodes (nodes with children)
- [x] Resolution independence
- []  Watch shader files for changes (auto) reload them. It broked.
- [x] Importing Meshes (tinyobjloader)
- [x] Deduplication of imported Meshes
- [x] Immediate mode GUI (imgui)
- [ ] Use quaterions for transformations
- [ ] Implement a job system
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
- [x] Multiple lights - uniform buffers
- [ ] Per object diffuse texture - texture buffers (?)
- [ ] Translate normals and trnasform them if object moves
- [ ] Blinn-Phong shading
- [ ] Light material: light power, light color,

Code hygiene
- [ ] Prefix all functions that touch OpenGL with "gl_"
- [ ] Change all OpenGL specific location from uint64_t to uint32_t since OpenGL maps it's indices to int (32 bits).

# License
The MIT License (MIT)
Copyright (c) 2017 Alexander Lingtorp

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
