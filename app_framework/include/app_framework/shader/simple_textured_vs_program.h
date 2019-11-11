//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once

namespace ml {
namespace app_framework {

static const char *kSimpleTexturedVertexShader = R"GLSL(
  #version 410 core

  layout(std140) uniform Transforms {
    mat4 view_proj;
    mat4 model;
  } transforms;

  layout (location = 0) in vec3 position;
  layout (location = 1) in vec2 tex_coords;

  out gl_PerVertex {
      vec4 gl_Position;
  };

  layout (location = 0) out vec2 out_tex_coords;

  void main() {
    gl_Position = transforms.view_proj * transforms.model * vec4(position, 1.0);
    out_tex_coords = tex_coords;
  }
)GLSL";

}
}
