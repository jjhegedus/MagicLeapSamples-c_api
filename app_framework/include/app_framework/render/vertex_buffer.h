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
#include "gl_type_size.h"

#include <string>

namespace ml {
namespace app_framework {

class VertexBuffer final : public Buffer {
public:
  VertexBuffer(Buffer::Category category, GLint element_type, uint64_t element_cnt)
      : VertexBuffer("", category, element_type, element_cnt) {
  }

  VertexBuffer(const std::string& name, Buffer::Category category, GLint element_type, uint64_t element_cnt)
      : Buffer(category, GL_ARRAY_BUFFER), name_(name), element_type_(element_type), element_cnt_(element_cnt) {
    element_size_ = GetGLTypeSize(element_type);
    vertex_size_ = element_size_ * element_cnt_;
    vertex_cnt_ = 0;
  }
  ~VertexBuffer() = default;

  void UpdateBuffer(const char *data, uint64_t size) override {
    Buffer::UpdateBuffer(data, size);
    vertex_cnt_ = size / vertex_size_;
  }

  uint64_t GetElementCount() const {
    return element_cnt_;
  }

  GLint GetElementType() const {
    return element_type_;
  }

  uint64_t GetElementSize() const {
    return element_size_;
  }

  uint64_t GetVertexCount() const {
    return vertex_cnt_;
  }

  uint64_t GetVertexSize() const {
    return vertex_size_;
  }

  const std::string& GetName() const {
    return name_;
  }

  void SetName(const std::string& name) {
    name_ = name;
  }

private:
  GLint element_type_;
  uint64_t element_cnt_;
  uint64_t element_size_;
  uint64_t vertex_cnt_;
  uint64_t vertex_size_;
  std::string name_;
};
}
}
