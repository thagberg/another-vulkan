#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Light {
	vec3 pos;
	vec3 color;
};

//layout(location = 0) in vec3 fragColor;
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 fragPos;

layout(std140, set = 0, binding = 0) uniform UniformLight {
	uint numLights;
	Light lights[10];
} lbo;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 baseColor = texture(texSampler, fragTexCoord);
	//float d = dot(inNormal, gl_FragCoord);
	for (int i = 0; i < lbo.numLights; i++) {
		vec3 lightDir = normalize(lbo.lights[i].pos - fragPos);
		float d = dot(inNormal, lightDir);
		baseColor *= (d * vec4(lbo.lights[i].color, 1.0f));
	}
	outColor = baseColor;
}