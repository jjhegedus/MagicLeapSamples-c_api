//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <unordered_map>
#include <vector>

#include <app_framework/common.h>
#include "fragment_program.h"
#include "geometry_program.h"
#include "uniform_buffer.h"
#include "variable.h"
#include "vertex_program.h"

namespace ml {
namespace app_framework {

#define MATERIAL_VARIABLE_DECLARE(type, name) \
  inline void Set##name(const type &value) {  \
    auto variable = GetVariable(#name);       \
    if (variable) variable->SetValue(value);  \
    dirty_ = true;                            \
  }                                           \
  inline type Get##name() const {             \
    auto variable = GetVariable(#name);       \
    if (!variable) return type();             \
    return variable->GetValue<type>();        \
  }

// The shading information that is required for rendering
class Material {
public:
  Material() : dirty_(true) {}
  Material(const Material& rhs);
  virtual ~Material() = default;

  std::shared_ptr<VertexProgram> GetVertexProgram() const {
    return vert_;
  }
  std::shared_ptr<GeometryProgram> GetGeometryProgram() const {
    return geom_;
  }
  std::shared_ptr<FragmentProgram> GetFragmentProgram() const {
    return frag_;
  }

  // Generic material property interface
  inline std::shared_ptr<Variable> GetVariable(const std::string &name) const {
    auto it = variables_by_name_.find(name);
    if (it == variables_by_name_.end()) {
      return nullptr;
    }
    return it->second;
  }

  void SetVertexProgram(std::shared_ptr<VertexProgram> program) {
    vert_ = program;
  }
  void SetGeometryProgram(std::shared_ptr<GeometryProgram> program) {
    geom_ = program;
  }
  void SetFragmentProgram(std::shared_ptr<FragmentProgram> program) {
    frag_ = program;
    BuildVariable();
  }

  std::shared_ptr<UniformBuffer> UpdateMaterialUniformBuffer();
  void UpdateMaterialUniforms();

protected:
  bool dirty_;
private:
  void BuildVariable();

  std::shared_ptr<FragmentProgram> frag_;
  std::shared_ptr<GeometryProgram> geom_;
  std::shared_ptr<VertexProgram> vert_;

  std::unordered_map<std::string, std::shared_ptr<Variable>> variables_by_name_;
  std::shared_ptr<UniformBuffer> buffer_;
  std::vector<char> ubo_cache_;
  UniformBlockDescription blk_desc_;
  std::vector<UniformDescription> textures_des_;
};
}
}
