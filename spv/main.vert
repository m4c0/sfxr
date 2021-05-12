#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(push_constant) uniform upc {
  vec2 pixels;
} m;

layout(location = 0) in vec2 vertex;
layout(location = 1) in ivec2 instance;
layout(location = 2) in uvec4 color;

layout(location = 0) out vec4 frag_color;

void main() {
  gl_Position = vec4((vertex + instance) / m.pixels - 1, 0, 1);
  frag_color = color / 256.0;
}

