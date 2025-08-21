#version 450

layout(binding = 0, set = 2) uniform sampler2D tex;

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 frag_uv;
layout(location = 1) in vec3 frag_norm;

const vec3 SUN = normalize(vec3(-1.0, 1.0, 1.0));

void main() {
	vec3 N = normalize(frag_norm);
	vec4 samp = texture(tex, frag_uv);
	vec3 albedo = samp.rgb;
	albedo *= clamp(dot(N, SUN), 0.0, 1.0);
	out_color = vec4(albedo, samp.a);
}