#pragma once

#include "m4c0/fuji/main_loop.hpp"

class stuff : public m4c0::fuji::main_loop_listener {
public:
  explicit stuff(const m4c0::fuji::device_context * ld) {
  }

  void build_primary_command_buffer(VkCommandBuffer cb) {
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) {
  }
  void on_render_extent_change(m4c0::vulkan::extent_2d e) {
  }
};
