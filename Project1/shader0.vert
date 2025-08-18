#version 450

layout(location = 0) in vec3 vert_pos;
layout(location = 1) in vec2 vert_uv;

layout(binding = 0, set = 1) uniform UniformData0 {
	mat4x4 eyeproj;
	mat4x4 invworld;
} ud0;

layout(location = 0) out vec2 frag_uv;

void main() {
	gl_Position = ud0.eyeproj * ud0.invworld * vec4(vert_pos, 1.0);
	frag_uv = vert_uv;
}