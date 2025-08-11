#version 450

layout(location = 0) in vec3 vert_pos;
layout(location = 1) in vec3 vert_col;

struct UniformData0 {
	mat4x4 eyeproj;
};

layout(location = 0) uniform UniformData0 ud0;

layout(location = 0) out vec3 frag_color;

void main() {
	gl_Position = ud0.eyeproj * vec4(vert_pos, 1.0);
	frag_color = vert_col;
}