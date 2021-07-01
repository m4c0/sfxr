#include "m4c0.vulkan.hpp"
#include "m4c0/casein/fuji.hpp"
#include "m4c0/fuji/main_loop_thread.hpp"

#include <memory>

extern "C" {
#include "main.frag.h"
#include "main.vert.h"
}

pipeline::pipeline(const scr_context * ctx)
  : m_pipeline_layout(
      pipeline_layout::builder().add_vertex_push_constant_with_size_and_offset(sizeof(consts), 0).build())
  , m_pipeline(pipe::builder()
                   .with_pipeline_layout(&m_pipeline_layout)
                   .with_render_pass(ctx->ld->render_pass())
                   .without_depth_test()
                   .add_vertex_binding_with_stride(vtx_mem::stride())
                   .add_vertex_binding_instanced_with_stride(color_mem::stride())
                   .add_vec2_attribute_with_bind_and_offset(0, 0)
                   .add_s16vec4_attribute_with_bind_and_offset(1, 0)
                   .add_u8vec4_attribute_with_bind_and_offset(1, sizeof(std::uint64_t))
                   .add_vertex_stage(shader(main_vert_spv, main_vert_spv_len), "main")
                   .add_fragment_stage(shader(main_frag_spv, main_frag_spv_len), "main")
                   .build())
  , m_consts({ static_cast<float>(ctx->w) / 2, static_cast<float>(ctx->h) / 2 }) {
}

std::unique_ptr<m4c0::casein::handler> m4c0::casein::main(const m4c0::native_handles * nh) {
  return std::make_unique<m4c0::casein::fuji_handler<loop>>(nh);
}
