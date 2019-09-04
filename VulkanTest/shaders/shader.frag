#version 450
#extension GL_ARB_separate_shader_objects : enable

struct AmbientLight {
	vec3 color;
	float intensity;
};

struct Light {
	vec3 pos;
	vec3 color;
	float intensity;
};

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 fragPos;

layout(std140, set = 0, binding = 0) uniform UniformLight {
	uint numLights;
	Light lights[10];
	AmbientLight ambient;
} lbo;

layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 modelViewProj;
	vec3 cameraPos;
} ubo;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 viewDir = normalize(ubo.cameraPos - fragPos);
	vec4 baseColor = texture(texSampler, fragTexCoord);
	vec3 ambientLight = lbo.ambient.intensity * lbo.ambient.color;
	vec3 diffuseLight = vec3(0.f, 0.f, 0.f);
	vec3 specularLight = vec3(0.f, 0.f, 0.f);
	for (int i = 0; i < lbo.numLights; i++) {
		Light thisLight = lbo.lights[i];
		vec3 lightDir = normalize(thisLight.pos - fragPos);
		vec3 reflectDir = reflect(-lightDir, inNormal);
		float d = thisLight.intensity * max(dot(inNormal, lightDir), 0);
		float s = pow(max(dot(viewDir, reflectDir), 0.f), 32);
		diffuseLight += d * thisLight.color;
		specularLight += 0.5f * s * thisLight.color;
	}
	outColor = vec4((ambientLight + diffuseLight + specularLight), 1.f) * baseColor;
	//outColor = vec4(ubo.cameraPos, 1.f);
}