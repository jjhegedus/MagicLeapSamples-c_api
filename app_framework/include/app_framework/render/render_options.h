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

namespace ml {
namespace app_framework {

// Render options and states
struct RenderOptions final {
  RenderOptions() : primitives(GL_TRIANGLES), fillmode(GL_FILL) {}

  GLint primitives;
  GLint fillmode;
  GLfloat point_size;
};

}
}
