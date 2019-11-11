//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include "mesh.h"

namespace ml {
namespace app_framework {

Mesh::Mesh(Buffer::Category buffer_category, GLint index_buffer_element_type) : num_vertices_(0) {
  vert_buffer_ = std::make_shared<VertexBuffer>(VertexAttributeName::kPosition, buffer_category, GL_FLOAT, 3);
  normal_buffer_ = std::make_shared<VertexBuffer>(VertexAttributeName::kNormal, buffer_category, GL_FLOAT, 3);
  tex_coords_buffer_ = std::make_shared<VertexBuffer>(VertexAttributeName::kTextureCoordinates, buffer_category, GL_FLOAT, 2);
  index_buffer_ = std::make_shared<IndexBuffer>(buffer_category, index_buffer_element_type);
}
}
}
