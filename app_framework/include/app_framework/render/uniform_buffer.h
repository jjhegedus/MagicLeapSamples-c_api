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
#include "buffer.h"

namespace ml {
namespace app_framework {

class UniformBuffer final : public Buffer {
public:
  UniformBuffer(Buffer::Category category) : Buffer(category, GL_UNIFORM_BUFFER) {}
  ~UniformBuffer() = default;
};
}
}
