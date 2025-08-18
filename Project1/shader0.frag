#version 450

layout(binding = 0, set = 2) uniform sampler2D tex;

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 frag_uv;

void main() {
	vec4 samp = texture(tex, frag_uv);
	out_color = samp;
	//out_color = vec4(frag_uv, 0.0, 1.0);
}