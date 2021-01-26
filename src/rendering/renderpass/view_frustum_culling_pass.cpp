#include "view_frustum_culling_pass.hpp"

#include "../graphicsbatch.hpp"
#include "../renderer.hpp"
#include "../shader.hpp"
#include "../../math/vector.hpp"
#include "../../rendering/primitives.hpp"
#include "../../util/filesystem.hpp"
#include "../../nodes/model.hpp"
#include "gbuffer_pass.hpp"
#include "../shader.hpp"

#include <array>

#include <GL/glew.h>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/// Returns the planes from the frustrum matrix in order; {left, right, bottom, top, near, far} (MV = world space with normal poiting to positive halfspace)
/// See: http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
std::array<glm::vec4, 6> extract_planes(const glm::mat4& mat) {
  const auto left_plane = glm::vec4(mat[3][0] + mat[0][0],
    mat[3][1] + mat[0][1],
    mat[3][2] + mat[0][2],
    mat[3][3] + mat[0][3]);

  const auto right_plane = glm::vec4(mat[3][0] - mat[0][0],
    mat[3][1] - mat[0][1],
    mat[3][2] - mat[0][2],
    mat[3][3] - mat[0][3]);

  const auto bot_plane = glm::vec4(mat[3][0] + mat[1][0],
    mat[3][1] + mat[1][1],
    mat[3][2] + mat[1][2],
    mat[3][3] + mat[1][3]);

  const auto top_plane = glm::vec4(mat[3][0] - mat[1][0],
    mat[3][1] - mat[1][1],
    mat[3][2] - mat[1][2],
    mat[3][3] - mat[1][3]);

  const auto near_plane = glm::vec4(mat[3][0] + mat[2][0],
    mat[3][1] + mat[2][1],
    mat[3][2] + mat[2][2],
    mat[3][3] + mat[2][3]);

  const auto far_plane = glm::vec4(mat[3][0] - mat[2][0],
    mat[3][1] - mat[2][1],
    mat[3][2] - mat[2][2],
    mat[3][3] - mat[2][3]);
  return { normalize(left_plane), normalize(right_plane), normalize(bot_plane), normalize(top_plane), normalize(near_plane), normalize(far_plane) };
}

bool ViewFrustumCullingRenderPass::setup(Renderer* render) {
  shader = new ComputeShader(Filesystem::base + "shaders/culling.comp.glsl");
  // TODO: Error checking?
  return true;
}

bool ViewFrustumCullingRenderPass::render(Renderer* render) {
  render->pass_started("Culling pass");

  // NOTE: Extraction of frustum planes are performed on the transpose (because of column/row-major difference).
  // FIXME: Use the Direct3D way of extraction instead since GLM appears to store the matrix in a row-major way.
  const glm::mat4 proj_view = render->projection_matrix * render->camera_transform;
  const std::array<glm::vec4, 6> frustum = extract_planes(glm::transpose(proj_view));

  glUseProgram(shader->gl_program);
  glUniform4fv(glGetUniformLocation(shader->gl_program, "frustum_planes"), 6, glm::value_ptr(frustum[0]));
  for (size_t i = 0; i < render->graphics_batches.size(); i++) {
    const auto& batch = render->graphics_batches[i];

    // Rebind the draw cmd SSBO for the compute shader to the current batch
    const uint32_t gl_draw_cmd_binding_point = 0; // Defaults to 0 in the culling compute shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_draw_cmd_binding_point, batch.gl_ibo);

    const uint32_t gl_instance_idx_binding_point = 1; // Defaults to 1 in the culling compute shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_instance_idx_binding_point, batch.gl_instance_idx_buffer);

    const uint32_t gl_bounding_volume_binding_point = 5; // Defaults to 5 in the culling compute shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, gl_bounding_volume_binding_point, batch.gl_bounding_volume_buffer);

    glUniform1ui(glGetUniformLocation(shader->gl_program, "NUM_INDICES"), batch.mesh->indices.size());
    glUniform1ui(glGetUniformLocation(shader->gl_program, "DRAW_CMD_IDX"), batch.gl_curr_ibo_idx);

    glDispatchCompute(batch.objects.transforms.size(), 1, 1);
  }
  // TODO: Is this barrier required?
  glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT); // Buffer objects affected by this bit are derived from the GL_DRAW_INDIRECT_BUFFER binding.

  render->pass_ended();

  return true;
}
