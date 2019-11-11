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

#include <app_framework/common.h>
#include "index_buffer.h"
#include "texture.h"
#include "uniform_buffer.h"
#include "vertex_buffer.h"
#include "program.h"

namespace ml {
namespace app_framework {

class ResourcePool;

// Geometry data
class Mesh : public RuntimeType {
  RUNTIME_TYPE_REGISTER(Mesh)
public:
  Mesh(Buffer::Category buffer_category, GLint index_buffer_element_type = GL_UNSIGNED_INT);
  virtual ~Mesh() = default;

  std::shared_ptr<VertexBuffer> GetVertexBuffer() const {
    return vert_buffer_;
  }

  std::shared_ptr<VertexBuffer> GetNormalBuffer() const {
    return normal_buffer_;
  }

  std::shared_ptr<VertexBuffer> GetTextureCoordinatesBuffer() const {
    return tex_coords_buffer_;
  }

  std::shared_ptr<IndexBuffer> GetIndexBuffer() const {
    return index_buffer_;
  }

  void SetCustomBuffer(const std::string& name, std::shared_ptr<VertexBuffer> buffer) {
    buffer->SetName(name);
    custom_buffers_.push_back(buffer);
  }

  const std::vector<std::shared_ptr<VertexBuffer>>& GetCustomBuffers() const {
    return custom_buffers_;
  }

  void UpdateMesh(glm::vec3 const *vertices, glm::vec3 const *normals, size_t num_vertices, void const *indices,
                  size_t num_indices) {
    if (vertices) {
      vert_buffer_->UpdateBuffer((char *)vertices, num_vertices * 3 * sizeof(float));
    }
    if (normals) {
      normal_buffer_->UpdateBuffer((char *)normals, num_vertices * 3 * sizeof(float));
    }
    if (indices) {
      index_buffer_->UpdateBuffer((char *)indices, num_indices * index_buffer_->GetIndexSize());
    }
    num_vertices_ = num_vertices;
  }

  void UpdateTexCoordsBuffer(glm::vec2 const *tex_coords) {
    if (tex_coords) {
      tex_coords_buffer_->UpdateBuffer((char *)tex_coords, num_vertices_ * 2 * sizeof(float));
    }
  }

private:
  std::shared_ptr<VertexBuffer> normal_buffer_;
  std::shared_ptr<IndexBuffer> index_buffer_;
  std::shared_ptr<VertexBuffer> vert_buffer_;
  std::shared_ptr<VertexBuffer> tex_coords_buffer_;
  size_t num_vertices_;

  std::vector<std::shared_ptr<VertexBuffer>> custom_buffers_;
};
}
}
