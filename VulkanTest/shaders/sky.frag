#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform samplerCube samplerCubeMap;

layout(location = 0) in vec3 inUVW;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform GammaSettings {
	float gamma;
	bool sRGB;
} gammaSettings;

void main() {
	vec4 sampledColor = texture(samplerCubeMap, inUVW);
	if (gammaSettings.sRGB) {
		sampledColor.rgb = pow(sampledColor.rgb, vec3(gammaSettings.gamma));
	}
	outColor = vec4(pow(sampledColor.rgb, vec3(1.0/gammaSettings.gamma)), sampledColor.a);
}
