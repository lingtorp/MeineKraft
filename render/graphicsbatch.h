#ifndef MEINEKRAFT_GRAPHICSBATCH_H
#define MEINEKRAFT_GRAPHICSBATCH_H

#include "primitives.h"

class RenderComponent;

/**
 * Contains the rendering context for a given set of geometry data
 * RenderComponents are batched in to a GraphicsBatch based on this geometry data
 */
class GraphicsBatch {
public:
  explicit GraphicsBatch(uint64_t mesh_id);

  /// Id given to each unique mesh loaded by MeshManager
  uint64_t mesh_id;
  
  Mesh mesh;
  
  // TODO Docs
  std::vector<RenderComponent *> components;
  
  // TODO Docs
  uint32_t gl_VAO;
  uint32_t gl_models_buffer_object;
  uint32_t gl_camera_view;
  uint32_t gl_camera_position;

  // TODO Docs
  Shader shader;
};

#endif // MEINEKRAFT_GRAPHICSBATCH_H
