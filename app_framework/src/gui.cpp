//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include "app_framework/gui.h"

#include <app_framework/convert.h>

#include <app_framework/components/renderable_component.h>
#include <app_framework/render/texture.h>

#include <app_framework/geometry/quad_mesh.h>
#include <app_framework/material/textured_material.h>

#include <ml_input.h>

namespace ml {
namespace app_framework {

static constexpr int32_t kImguiQuadWidth = 540;
static constexpr int32_t kImguiQuadHeight = 540;
static constexpr float kCursorSpeed = kImguiQuadWidth * 10.f;
static constexpr float kPressThreshold = 0.5f;

Gui::Gui()
    : owned_input_(false),
      input_handle_(ML_INVALID_HANDLE),
      prev_touch_pos_and_force_{},
      cursor_pos_{},
      imgui_color_texture_(0),
      imgui_framebuffer_(0),
      imgui_depth_renderbuffer_(0),
      state_(State::Hidden) {}

void Gui::Initialize(MLHandle input_handle) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  if (input_handle == ML_INVALID_HANDLE) {
    owned_input_ = true;
    MLInputCreate(nullptr, &input_handle_);
  } else {
    owned_input_ = false;
    input_handle_ = input_handle;
  }

  ImGui_ImplOpenGL3_Init("#version 410 core");

  // Setup back-end capabilities flags
  ImGuiIO &io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

  // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
  io.KeyMap[ImGuiKey_Tab] = MLKEYCODE_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = MLKEYCODE_DPAD_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = MLKEYCODE_DPAD_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = MLKEYCODE_DPAD_UP;
  io.KeyMap[ImGuiKey_DownArrow] = MLKEYCODE_DPAD_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = MLKEYCODE_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown] = MLKEYCODE_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home] = MLKEYCODE_HOME;
  io.KeyMap[ImGuiKey_End] = MLKEYCODE_ENDCALL;
  io.KeyMap[ImGuiKey_Insert] = MLKEYCODE_INSERT;
  io.KeyMap[ImGuiKey_Delete] = MLKEYCODE_DEL;
  io.KeyMap[ImGuiKey_Backspace] = MLKEYCODE_BACK;
  io.KeyMap[ImGuiKey_Space] = MLKEYCODE_SPACE;
  io.KeyMap[ImGuiKey_Enter] = MLKEYCODE_ENTER;
  io.KeyMap[ImGuiKey_Escape] = MLKEYCODE_ESCAPE;
  io.KeyMap[ImGuiKey_A] = MLKEYCODE_A;
  io.KeyMap[ImGuiKey_C] = MLKEYCODE_C;
  io.KeyMap[ImGuiKey_V] = MLKEYCODE_V;
  io.KeyMap[ImGuiKey_X] = MLKEYCODE_X;
  io.KeyMap[ImGuiKey_Y] = MLKEYCODE_Y;
  io.KeyMap[ImGuiKey_Z] = MLKEYCODE_Z;

  glGenFramebuffers(1, &imgui_framebuffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, imgui_framebuffer_);

  glGenTextures(1, &imgui_color_texture_);
  glBindTexture(GL_TEXTURE_2D, imgui_color_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kImguiQuadWidth, kImguiQuadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  off_screen_texture_ =
      std::make_shared<Texture>(GL_TEXTURE_2D, imgui_color_texture_, kImguiQuadWidth, kImguiQuadHeight, false);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, imgui_color_texture_, 0);

  glGenRenderbuffers(1, &imgui_depth_renderbuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, imgui_depth_renderbuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, kImguiQuadWidth, kImguiQuadHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, imgui_depth_renderbuffer_);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    ML_LOG(Fatal, "Framebuffer is not complete!");
  }
  ImGui::StyleColorsDark();

  std::shared_ptr<Mesh> quad = Registry::GetInstance()->GetResourcePool()->GetMesh<QuadMesh>();
  std::shared_ptr<TexturedMaterial> gui_mat = std::make_shared<TexturedMaterial>(off_screen_texture_);
  std::shared_ptr<RenderableComponent> gui_renderable = std::make_shared<RenderableComponent>(quad, gui_mat);
  gui_node_ = std::make_shared<Node>();
  gui_node_->AddComponent(gui_renderable);

  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(kImguiQuadWidth), static_cast<float>(kImguiQuadHeight)), ImGuiCond_FirstUseEver);
}

void Gui::Cleanup() {
  glDeleteTextures(1, &imgui_color_texture_);
  glDeleteFramebuffers(1, &imgui_framebuffer_);
  glDeleteRenderbuffers(1, &imgui_depth_renderbuffer_);
  ImGui_ImplOpenGL3_Shutdown();
  if (owned_input_) {
    MLInputDestroy(input_handle_);
    input_handle_ = 0;
  }
}

void Gui::BeginUpdate() {
  ImGuiIO &io = ImGui::GetIO();

  io.DisplaySize = ImVec2((float)kImguiQuadWidth, (float)kImguiQuadHeight);
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

  // Setup time step
  auto current_time = std::chrono::steady_clock::now();
  std::chrono::duration<float> delta_time = current_time - previous_time_;
  io.DeltaTime = delta_time.count();
  previous_time_ = current_time;
  io.MouseDrawCursor = true;

  // Handle input
  MLInputControllerState input_states[MLInput_MaxControllers] = {};
  MLInputGetControllerState(input_handle_, input_states);
  UpdateState(input_states[0]);
  if (state_ != State::Hidden) {
    UpdateMousePosAndButtons(input_states[0]);
  }

  // Start the ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  IM_ASSERT(io.Fonts->IsBuilt());
  ImGui::NewFrame();

  // Set default position and size for new windows
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(kImguiQuadWidth), static_cast<float>(kImguiQuadHeight)), ImGuiCond_FirstUseEver);
}

void Gui::EndUpdate() {
  ImGui::Render();

  // Off-screen render
  glBindFramebuffer(GL_FRAMEBUFFER, imgui_framebuffer_);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClearDepth(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Gui::UpdateState(const MLInputControllerState &input_state) {
  bool toggle_state = input_state.button_state[MLInputControllerButton_Bumper];
  if (toggle_state && !prev_toggle_state_) {
    switch (state_) {
      case State::Hidden: state_ = State::Moving; break;
      case State::Moving: state_ = State::Placed; break;
      case State::Placed: state_ = State::Hidden; break;
    }
  }
  prev_toggle_state_ = toggle_state;

  switch (state_) {
    case State::Hidden: {
      gui_node_->GetComponent<RenderableComponent>()->SetVisible(false);
      break;
    }
    case State::Moving: {
      const auto rotation = to_glm(input_state.orientation);
      const auto translation = to_glm(input_state.position) + rotation * glm::vec3(0.f, 0.f, -1.f);

      gui_node_->GetComponent<RenderableComponent>()->SetVisible(true);
      gui_node_->SetWorldRotation(rotation);
      gui_node_->SetWorldTranslation(translation);
      gui_node_->SetLocalScale(glm::vec3(0.75f, 0.75f, 0));
      break;
    }
    case State::Placed: {
      gui_node_->GetComponent<RenderableComponent>()->SetVisible(true);
      break;
    }
  }
}

void Gui::UpdateMousePosAndButtons(const MLInputControllerState &input_state) {
  // Update buttons
  ImGuiIO &io = ImGui::GetIO();

  bool prev_mouse_down[5] = {};
  std::memcpy(prev_mouse_down, io.MouseDown, 5);

  io.MouseDown[0] = input_state.trigger_normalized > kPressThreshold;

  for (size_t i = 0; i < 5; ++i) {
    if (io.MouseDown[i] && !prev_mouse_down[i]) {
      MLInputStartControllerFeedbackPatternVibe(input_handle_, 0, MLInputControllerFeedbackPatternVibe_Click,
                                                MLInputControllerFeedbackIntensity_High);
    }
  }
  if (input_state.touch_pos_and_force[0].z > 0.f && prev_touch_pos_and_force_.z > 0.f) {
    float delta_x = (input_state.touch_pos_and_force[0].x - prev_touch_pos_and_force_.x);
    float delta_y = (input_state.touch_pos_and_force[0].y - prev_touch_pos_and_force_.y);
    glm::vec2 delta_pos = io.DeltaTime * kCursorSpeed * glm::vec2(delta_x, -delta_y);
    static const glm::vec2 bounds = glm::vec2(kImguiQuadWidth, kImguiQuadHeight);
    cursor_pos_ = glm::clamp(cursor_pos_ + delta_pos, glm::vec2(0), bounds);
  }
  io.MousePos = ImVec2(cursor_pos_.x, cursor_pos_.y);
  prev_touch_pos_and_force_ = input_state.touch_pos_and_force[0];
}

}  // namespace app_framework
}  // namespace ml
