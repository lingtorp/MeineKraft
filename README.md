# MeineKraft
## - Study of 3D game engine with a Minecraft clone
 Written in C++11 & OpenGL, also an adventure into C++ and OpenGL.
 ![](/screenshots/perlin-hills.gif)
 ![](/screenshots/linear-fog.gif)

# Notebook
* Procedural generation
    * Perlin Noise

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
- [ ] Line of sight distance
- [ ] View frustrum culling (geometric approach)
- [ ] Ray tracing for entity selection
- [ ] Merge all 3 transformation matrices into one
- [ ] Frustrum cull checks against chunks
