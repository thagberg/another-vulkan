#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform samplerCube samplerCubeMap;

layout(location = 0) in vec3 inUVW;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform SkySettings {
	float gamma;
	float lod;
} skySettings;

void main() {
	vec4 sampledColor = textureLod(samplerCubeMap, inUVW, skySettings.lod);
	//if (gammaSettings.sRGB) {
		//sampledColor.rgb = pow(sampledColor.rgb, vec3(2.2));
	//}
	outColor = vec4(pow(sampledColor.rgb, vec3(1.0/skySettings.gamma)), sampledColor.a);
	//outColor = vec4(vec3(skySettings.lod), 1.0);
}
