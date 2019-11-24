#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform sampler2D quadSampler;

layout(location = 0) in vec2 inUV;

layout (push_constant) uniform ExposureSettings {
	float value;
} exposure;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec3 hdrColor = texture(quadSampler, inUV).rgb;
	vec3 tonemappedColor = vec3(1.0) - exp(-hdrColor * exposure.value);
	// Reinhard tone mapping
	//vec3 tonemappedColor = hdrColor / (hdrColor + vec3(1.0));
    outColor = vec4(tonemappedColor, 1.0);
}
