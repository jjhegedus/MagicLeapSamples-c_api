//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include "vertex_program.h"
#include "gl_type_size.h"

namespace ml {
namespace app_framework {

VertexProgram::VertexProgram(const char *code) : Program(code, GL_VERTEX_SHADER) {
  // Parse parameters
  std::vector<GLchar> name_buffer(512);

  glGetProgramiv(GetGLProgram(), GL_ACTIVE_ATTRIBUTES, &att_cnt_);
  for (int32_t i = 0; i < att_cnt_; ++i) {
    VertexAttributeDescription data{};
    data.index = i;
    GLsizei char_written = 0;
    glGetActiveAttrib(GetGLProgram(), i, name_buffer.size(), &char_written, (GLint *)&data.element_cnt, &data.type,
                      name_buffer.data());
    data.location = glGetAttribLocation(GetGLProgram(), name_buffer.data());
    data.name = std::string((char *)name_buffer.data(), char_written);

    vertex_attrs_by_name_[data.name] = data;

    data.size = GetGLTypeSize(data.type) * data.element_cnt;
    ML_LOG(Debug, "Found vertex attribute: name(%s), index(%u) size(%" PRIu64 ") type(%x) element_cnt(%u) location(%u)",
           data.name.c_str(), data.index, data.size, data.type, data.element_cnt, data.location);
  }
}
}
}
