//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <tuple>
#include <unordered_map>
#include <functional>

#include <app_framework/common.h>
#include <app_framework/node.h>
#include <app_framework/components/camera_component.h>
#include <app_framework/components/renderable_component.h>
#include <app_framework/components/light_component.h>
#include "fragment_program.h"
#include "geometry_program.h"
#include "vertex_program.h"

namespace ml {
namespace app_framework {
typedef std::tuple<GLuint, GLuint, GLuint> ShaderKey;
}
}
namespace std {
template <>
struct hash<ml::app_framework::ShaderKey> {
  std::size_t operator()(const ml::app_framework::ShaderKey &key) const {
    return (int)std::get<0>(key) << 16 & (int)std::get<1>(key) << 16 << (int)std::get<2>(key);
  }
};
}

namespace ml {
namespace app_framework {

// Basic Transforms UBO
struct TransformsUBO {
  TransformsUBO(
    const glm::mat4& in_view_proj,
    const glm::mat4& in_model,
    const glm::mat4& in_model_view,
    const glm::vec3& in_camera_position)
    : view_proj(in_view_proj),
      model(in_model),
      model_view(in_model_view),
      camera_position(in_camera_position),
      pad0(0) {}
  glm::mat4 view_proj;
  glm::mat4 model;
  glm::mat4 model_view;
  glm::vec3 camera_position;
  float pad0;
};

struct Light {
  Light() {}
  Light(
    const glm::vec3& in_light_position,
    const glm::vec3& in_light_color,
    const glm::vec3& in_light_direction,
    LightType type,
    float strength)
    : light_position(in_light_position),
      light_color(in_light_color),
      light_direction(in_light_direction),
      light_type((int)type),
      light_strength(strength) {}
  glm::vec3 light_position;
  float light_strength;
  glm::vec3 light_direction;
  int32_t light_type;
  glm::vec3 light_color;
  float pad;
};

struct LightsUBO {
  LightsUBO(
    const std::vector<Light> &in_lights_array
  ) {
    int lightCnt = in_lights_array.size() > LightComponent::MAXIMUM_LIGHTS ? LightComponent::MAXIMUM_LIGHTS : in_lights_array.size();
    memcpy(lights_array, in_lights_array.data(), sizeof(Light) * lightCnt);
    number_of_lights = lightCnt;
  }

  Light lights_array[LightComponent::MAXIMUM_LIGHTS];
  int32_t number_of_lights;
};

// Renderer, runtime rendering
class Renderer final {
public:
  Renderer();
  ~Renderer();

  void Initialize();

  void Visit(std::shared_ptr<Node> node) {
    auto renderable = node->GetComponent<RenderableComponent>();
    if (renderable) {
      QueueRendererable(renderable);
    }
    auto cam = node->GetComponent<CameraComponent>();
    if (cam) {
      QueueCamera(cam);
    }
    auto light = node->GetComponent<LightComponent>();
    if (light) {
      QueueLight(light);
    }
  }

  // Render the queued renderables
  void Render();

  void ClearQueues();

  void SetPreRenderCameraCallback(const std::function<void(std::shared_ptr<CameraComponent>)>& callback) { pre_cam_callback_ = callback;}
  void SetPostRenderCameraCallback(const std::function<void(std::shared_ptr<CameraComponent>)>& callback) { post_cam_callback_ = callback;}
  void SetPreRenderCallback(const std::function<void()>& callback) {pre_render_callback_ = callback; }
  void SetPostRenderCallback(const std::function<void()>& callback) {post_render_callback_ = callback; }

  inline std::shared_ptr<CameraComponent> GetCurrentCamera() const {
    return current_cam_;
  }

  inline std::shared_ptr<VertexProgram> GetCurrentVertexProgram() const {
    return current_vertex_program_;
  }

  inline std::shared_ptr<FragmentProgram> GetCurrentFragmentProgram() const {
    return current_frag_program_;
  }

  inline std::shared_ptr<GeometryProgram> GetCurrentGeometryProgram() const {
    return current_geom_program_;
  }

  inline GLuint GetCurrentPipeline() const {
    return program_pipeline_;
  }

  inline const std::vector<std::shared_ptr<LightComponent>>& GetCurrentLights() const {
    return queued_lights_;
  }

private:
  void Render(std::shared_ptr<RenderableComponent> renderable);

  inline void BindBuffer(const std::shared_ptr<VertexBuffer>& buffer,
                         const std::string& buffer_name,
                         const std::unordered_map<std::string, VertexAttributeDescription>& vertex_attr_list);

  inline void BindTransformUniform(std::shared_ptr<Program> program, const TransformsUBO& transforms_ubo);

  // Queue a camera as a render target
  void QueueCamera(std::shared_ptr<CameraComponent> camera);

  // Queue the renderable objects
  void QueueRendererable(std::shared_ptr<RenderableComponent> renderable);

  void QueueLight(std::shared_ptr<LightComponent> light);

  std::function<void(std::shared_ptr<CameraComponent>)> pre_cam_callback_;
  std::function<void(std::shared_ptr<CameraComponent>)> post_cam_callback_;
  std::function<void()> pre_render_callback_;
  std::function<void()> post_render_callback_;

  void BindProgram(GLuint vert, GLuint geom, GLuint frag);

  std::vector<std::shared_ptr<RenderableComponent>> queued_renderables_;
  std::vector<std::shared_ptr<CameraComponent>> queued_cameras_;
  std::vector<std::shared_ptr<LightComponent>> queued_lights_;
  std::shared_ptr<CameraComponent> current_cam_;
  std::shared_ptr<VertexProgram> current_vertex_program_;
  std::shared_ptr<FragmentProgram> current_frag_program_;
  std::shared_ptr<GeometryProgram> current_geom_program_;

  GLuint program_pipeline_;
  std::unordered_map<ShaderKey, GLuint> shader_program_cache_;

  std::shared_ptr<UniformBuffer> transform_uniform_buffer_;
  std::shared_ptr<UniformBuffer> light_uniform_buffer_;
  std::vector<Light> lights_;
  bool transform_uniform_buffer_dirty_;
  GLuint vertex_array_;
};

}
}
