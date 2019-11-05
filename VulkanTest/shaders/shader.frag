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
layout(location = 3) in mat3 inTBN;

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

layout(set = 1, binding = 1) uniform sampler2D diffuseSampler;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D normalSampler;

layout (push_constant) uniform PushConstant {
	float specularStrength;
	uint shininess;
} push;

layout(location = 0) out vec4 outColor;

vec3 getSchlickFresnel(float dotP, vec3 F0) {
	return F0 + (1.0f - F0) * pow(1.0 - dotP, 5);
}

void main() {
	vec3 viewDir = normalize(ubo.cameraPos - fragPos);
	vec4 baseColor = texture(diffuseSampler, fragTexCoord);
	vec4 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord);
	vec3 surfaceNormal = texture(normalSampler, fragTexCoord).rgb;
    surfaceNormal = normalize(surfaceNormal * 2.0 - 1.0);
    surfaceNormal = normalize(inTBN * surfaceNormal);
	vec3 ambientLight = lbo.ambient.intensity * lbo.ambient.color;
	vec3 diffuseLight = vec3(0.f, 0.f, 0.f);
	vec3 specularLight = vec3(0.f, 0.f, 0.f);

	vec4 dynamicColor = vec4(0.f, 0.f, 0.f, 0.f);
	for (int i = 0; i < lbo.numLights; i++) {
		Light thisLight = lbo.lights[i];
		vec3 lightDir = normalize(thisLight.pos - fragPos);
		//vec3 reflectDir = reflect(-lightDir, inNormal);
		vec3 reflectDir = reflect(-lightDir, surfaceNormal);
		vec3 halfVec = normalize(lightDir + viewDir);
		//float d = thisLight.intensity * max(dot(inNormal, lightDir), 0);
		float d = thisLight.intensity * max(dot(surfaceNormal, lightDir), 0);
		float s = thisLight.intensity * pow(max(dot(viewDir, reflectDir), 0.f), 32);


		specularLight += push.specularStrength * getSchlickFresnel(max(dot(halfVec, viewDir), 0), vec3(0.04)) * s;
		diffuseLight = (1.0 - specularLight) * thisLight.color * d;
		dynamicColor += vec4(diffuseLight, 1.f) * baseColor;
		dynamicColor += vec4(specularLight, 1.f);
	}
	vec4 ambientColor = vec4(ambientLight, 1.f) * baseColor;
	//outColor = ambientColor + dynamicColor;
    outColor = vec4(surfaceNormal, 1.0);
}