#pragma once

#include "m4c0/fuji/device_context.hpp"
#include "m4c0/fuji/main_loop.hpp"
#include "m4c0/vulkan/bind_pipeline.hpp"
#include "m4c0/vulkan/pipeline.hpp"
#include "m4c0/vulkan/pipeline_layout.hpp"
#include "m4c0/vulkan/shader_module.hpp"

#include <cstdint>
#include <span>

extern "C" {
#include "main.frag.h"
#include "main.vert.h"
}

template<class DataTp>
static auto shader(DataTp && data, unsigned size) {
  const auto * void_data = static_cast<const void *>(data);
  auto spv = std::span<const std::uint32_t>(static_cast<const std::uint32_t *>(void_data), size);
  return m4c0::vulkan::shader_module::create_from_spv(spv);
}

struct vertex {
  float x, y;
  float r, g, b, a;
};

class stuff : public m4c0::fuji::main_loop_listener {
  using pipeline = m4c0::vulkan::pipeline;
  using pipeline_layout = m4c0::vulkan::pipeline_layout;

  pipeline_layout m_pipeline_layout;
  pipeline m_pipeline;

public:
  explicit stuff(const m4c0::fuji::device_context * ld)
    : m_pipeline_layout(pipeline_layout::builder().build())
    , m_pipeline(pipeline::builder()
                     .with_pipeline_layout(&m_pipeline_layout)
                     .with_render_pass(ld->render_pass())
                     .add_vertex_binding_with_stride(sizeof(vertex))
                     .add_vec2_attribute_with_bind_and_offset(0, offsetof(vertex, x))
                     .add_vec4_attribute_with_bind_and_offset(0, offsetof(vertex, r))
                     .add_vertex_stage(shader(main_vert_spv, main_vert_spv_len), "main")
                     .add_fragment_stage(shader(main_frag_spv, main_frag_spv_len), "main")
                     .build()) {
  }

  void build_primary_command_buffer(VkCommandBuffer cb) override {
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) override {
    m4c0::vulkan::cmd::bind_pipeline(cb).with_pipeline(&m_pipeline).now();
  }
  void on_render_extent_change(m4c0::vulkan::extent_2d e) override {
  }
};
