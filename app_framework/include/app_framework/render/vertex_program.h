//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <app_framework/common.h>
#include "program.h"

namespace ml {
namespace app_framework {

// GL Vertex program
class VertexProgram final : public Program {
public:
  // Initialize the program with null-terminated code string
  VertexProgram(const char *code);
  virtual ~VertexProgram() = default;

  inline const std::unordered_map<std::string, VertexAttributeDescription>& GetVertexAttributes() const {
    return vertex_attrs_by_name_;
  }

private:
  GLint att_cnt_;
  std::unordered_map<std::string, VertexAttributeDescription> vertex_attrs_by_name_;
};
}
}
