#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 vertex;

layout(location = 0) out vec4 frag_color;

void main() {
  gl_Position = vec4(vertex, 0, 1);
  frag_color = vec4(0, 0, 1, 1);
}

