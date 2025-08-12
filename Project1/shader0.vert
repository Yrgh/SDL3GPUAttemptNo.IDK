#version 450

layout(location = 0) in vec3 vert_pos;
layout(location = 1) in vec3 vert_col;

layout(binding = 0, set = 1) uniform UniformData0 {
	mat4x4 eyeproj;
	mat4x4 invworld;
} ud0;

layout(location = 0) out vec3 frag_color;

void main() {
	gl_Position = ud0.eyeproj * ud0.invworld * vec4(vert_pos, 1.0);
	frag_color = vert_col;
}