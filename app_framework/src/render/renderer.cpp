//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include "renderer.h"

#include <algorithm>

namespace ml {
namespace app_framework {

Renderer::Renderer() : program_pipeline_(0), transform_uniform_buffer_dirty_(false) {}

void Renderer::Initialize() {
  glGenVertexArrays(1, &vertex_array_);
  transform_uniform_buffer_ = std::make_shared<UniformBuffer>(Buffer::Category::Dynamic);
  light_uniform_buffer_ = std::make_shared<UniformBuffer>(Buffer::Category::Dynamic);
}

Renderer::~Renderer() {}

void Renderer::QueueCamera(std::shared_ptr<CameraComponent> camera) {
  queued_cameras_.push_back(camera);
}

void Renderer::QueueRendererable(std::shared_ptr<RenderableComponent> renderable) {
  queued_renderables_.push_back(renderable);
}

void Renderer::QueueLight(std::shared_ptr<LightComponent> light) {
  queued_lights_.push_back(light);
}

void Renderer::Render() {
  // Simple single pass, per-object basis forward rendering
  if (pre_render_callback_) {
    pre_render_callback_();
  }

  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_FRAMEBUFFER_SRGB);

  for (const std::shared_ptr<CameraComponent> &cam : queued_cameras_) {
    current_cam_ = cam;
    if (pre_cam_callback_) {
      pre_cam_callback_(current_cam_);
    }

    // Bind the render target
    auto render_target = cam->GetRenderTarget();
    if (!render_target) {
      continue;
    }
    GLuint framebuffer = render_target->GetGLFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    auto viewport = current_cam_->GetViewport();
    glViewport((int)viewport.x, (int)viewport.y, (int)viewport.z, (int)viewport.w);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Sort back-to-front to allow for alpha blending
    const auto camera_position = cam->GetNode()->GetWorldTranslation();
    std::sort(queued_renderables_.begin(), queued_renderables_.end(),
              [&camera_position](const std::shared_ptr<RenderableComponent> &renderable1,
                                 const std::shared_ptr<RenderableComponent> &renderable2) {
                const auto dist1 = glm::distance(camera_position, renderable1->GetNode()->GetWorldTranslation());
                const auto dist2 = glm::distance(camera_position, renderable2->GetNode()->GetWorldTranslation());
                return dist1 > dist2;
              });

    for (const std::shared_ptr<RenderableComponent> &renderable : queued_renderables_) {
      if (!renderable->GetVisible()) {
        continue;
      }

      current_vertex_program_ = renderable->GetMaterial()->GetVertexProgram();
      current_frag_program_ = renderable->GetMaterial()->GetFragmentProgram();
      current_geom_program_ = renderable->GetMaterial()->GetGeometryProgram();

      bool bind_gs = true;
      if (!current_geom_program_) {
        bind_gs = false;
      } else if (renderable->options.primitives != current_geom_program_->GetInputPrimitiveType()) {
        bind_gs = false;
      }

      BindProgram(current_vertex_program_->GetGLProgram(), bind_gs ? current_geom_program_->GetGLProgram() : 0,
                  current_frag_program_->GetGLProgram());
      Render(renderable);
    }

    // Reset the global state to GL_FILL, on platform this is being
    // changed so it causes imgui to not render properly.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    auto blit_target = cam->GetBlitTarget();
    if (blit_target) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blit_target->GetGLFramebuffer());
      glBlitFramebuffer((int)viewport.x, (int)viewport.y, (int)viewport.z, (int)viewport.w, 0, 0,
                        blit_target->GetWidth(), blit_target->GetHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    if (post_cam_callback_) {
      post_cam_callback_(current_cam_);
    }
  }

  if (post_render_callback_) {
    post_render_callback_();
  }
  ClearQueues();
}

void Renderer::ClearQueues() {
  queued_renderables_.clear();
  queued_cameras_.clear();
  queued_lights_.clear();
}

void Renderer::BindProgram(GLuint vert, GLuint geom, GLuint frag) {
  std::tuple<GLuint, GLuint, GLuint> key = std::make_tuple(vert, geom, frag);
  auto it = shader_program_cache_.find(key);
  if (it == shader_program_cache_.end()) {
    glGenProgramPipelines(1, &program_pipeline_);
    glUseProgramStages(program_pipeline_, GL_VERTEX_SHADER_BIT, vert);
    if (geom != 0) {
      glUseProgramStages(program_pipeline_, GL_GEOMETRY_SHADER_BIT, geom);
    }
    glUseProgramStages(program_pipeline_, GL_FRAGMENT_SHADER_BIT, frag);
    shader_program_cache_[key] = program_pipeline_;
  } else {
    program_pipeline_ = it->second;
  }
  glBindProgramPipeline(program_pipeline_);
}

void Renderer::Render(std::shared_ptr<RenderableComponent> renderable) {
  auto lightsComp = GetCurrentLights();
  auto cam = GetCurrentCamera();
  auto model = renderable->GetNode()->GetWorldTransform();
  glm::mat4 view_proj = cam->GetProjectionMatrix() * glm::inverse(cam->GetNode()->GetWorldTransform());

  auto mesh = renderable->GetMesh();
  auto material = renderable->GetMaterial();

  // Vertex data
  const auto& vertex_attr_list = GetCurrentVertexProgram()->GetVertexAttributes();
  glBindVertexArray(vertex_array_);

  // Position
  auto vertex_buffer = mesh->GetVertexBuffer();
  BindBuffer(vertex_buffer, VertexAttributeName::kPosition, vertex_attr_list);

  // Normal
  auto normal_buffer = mesh->GetNormalBuffer();
  BindBuffer(normal_buffer, VertexAttributeName::kNormal, vertex_attr_list);

  // Texture coordinate
  auto texture_coords_buffer = mesh->GetTextureCoordinatesBuffer();
  BindBuffer(texture_coords_buffer, VertexAttributeName::kTextureCoordinates, vertex_attr_list);

  const auto& buffer_list = mesh->GetCustomBuffers();
  for (const auto& custom_buffer : buffer_list) {
    auto custom_it = vertex_attr_list.find(custom_buffer->GetName());
    if (custom_it != vertex_attr_list.end()) {
      const auto& attr = custom_it->second;
      glBindBuffer(GL_ARRAY_BUFFER, custom_buffer->GetGLBuffer());
      glVertexAttribPointer(attr.location, custom_buffer->GetElementCount(), custom_buffer->GetElementType(), GL_FALSE,
                            custom_buffer->GetVertexSize(), (void *)0);
      glEnableVertexAttribArray(attr.location);
    }
  }

  // Get the camera data, mvp, update the uniform
  transform_uniform_buffer_dirty_ = true;
  TransformsUBO transforms_ubo(
    view_proj,
    model,
    glm::inverse(cam->GetNode()->GetWorldTransform()) * model,
    cam->GetNode()->GetWorldTranslation());
  BindTransformUniform(GetCurrentVertexProgram(), transforms_ubo);
  BindTransformUniform(GetCurrentGeometryProgram(), transforms_ubo);
  BindTransformUniform(GetCurrentFragmentProgram(), transforms_ubo);

  const auto& fragment_ubo_blk_list = GetCurrentFragmentProgram()->GetUniformBlocks();
  // Light info
  lights_.clear();
  for (auto& light : lightsComp) {
    Light l(
      light->GetNode()->GetWorldTranslation(),
      light->GetLightColor(),
      light->GetDirection(),
      light->GetLightType(),
      light->GetLightStrength());
    lights_.push_back(l);
  }
  LightsUBO lights_ubo(lights_);
  auto fragment_ubo_light_it = fragment_ubo_blk_list.find(UniformName::kLight);
  if (fragment_ubo_light_it != fragment_ubo_blk_list.end()) {
    const auto& des = fragment_ubo_light_it->second;
    light_uniform_buffer_->UpdateBuffer((char *)&lights_ubo, sizeof(lights_ubo));
    glBindBufferBase(GL_UNIFORM_BUFFER, des.binding, light_uniform_buffer_->GetGLBuffer());
  }

  material->UpdateMaterialUniforms();
  auto fragment_ubo_it = fragment_ubo_blk_list.find(UniformName::kMaterial);
  if (fragment_ubo_it != fragment_ubo_blk_list.end()) {
    const auto& des = fragment_ubo_it->second;
    auto material_uniform_buffer = material->UpdateMaterialUniformBuffer();
    glBindBufferBase(GL_UNIFORM_BUFFER, des.binding, material_uniform_buffer->GetGLBuffer());
  }

  glPolygonMode(GL_FRONT_AND_BACK, renderable->options.fillmode);

  if (renderable->options.primitives == GL_POINTS) {
    glPointSize(renderable->options.point_size);
  }

  std::shared_ptr<IndexBuffer> index_buffer = mesh->GetIndexBuffer();
  if (renderable->options.primitives == GL_POINTS || !index_buffer || index_buffer->GetIndexCount() == 0) {
    glDrawArrays(renderable->options.primitives, 0, vertex_buffer->GetVertexCount());
  } else {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer->GetGLBuffer());
    glDrawElements(renderable->options.primitives, index_buffer->GetIndexCount(), index_buffer->GetIndexType(),
                   nullptr);
  }
}

void Renderer::BindTransformUniform(std::shared_ptr<Program> program, const TransformsUBO& transforms_ubo) {
  if (!program) {
    return;
  }
  const auto& vertex_ubo_list = program->GetUniformBlocks();
  // Bind the transform UBO
  auto vertex_ubo_it = vertex_ubo_list.find(UniformName::kTransforms);
  if (vertex_ubo_it != vertex_ubo_list.end()) {
    const auto& blk_des = vertex_ubo_it->second;
    if (transform_uniform_buffer_dirty_) {
      transform_uniform_buffer_->UpdateBuffer((char *)&transforms_ubo, sizeof(transforms_ubo));
      transform_uniform_buffer_dirty_ = false;
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, blk_des.binding, transform_uniform_buffer_->GetGLBuffer());
  }
}

void Renderer::BindBuffer(const std::shared_ptr<VertexBuffer>& buffer,
                                     const std::string& buffer_name,
                                     const std::unordered_map<std::string, VertexAttributeDescription>& vertex_attr_list) {
  if (!buffer || buffer->GetVertexCount() == 0) {
    return;
  }
  auto it = vertex_attr_list.find(buffer_name);
  if (it != vertex_attr_list.end()) {
    const auto& attr = it->second;
    glBindBuffer(GL_ARRAY_BUFFER, buffer->GetGLBuffer());
    glVertexAttribPointer(attr.location, buffer->GetElementCount(), buffer->GetElementType(), GL_FALSE,
                          buffer->GetVertexSize(), (void *)0);
    glEnableVertexAttribArray(attr.location);
  }
}

}  // namespace app_framework
}  // namespace ml
