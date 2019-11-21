#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D quadSampler;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform GammaSettings {
	float gamma;
} gammaSettings;

void main() 
{
	vec4 sampledColor = texture(quadSampler, inUV);
    outColor = vec4(pow(sampledColor.rgb, vec3(1.0/gammaSettings.gamma)), sampledColor.a);
}
