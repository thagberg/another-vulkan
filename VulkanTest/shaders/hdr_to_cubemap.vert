#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 modelViewProj;
	vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outLocalPos;

void main() {
	outLocalPos = inPosition;
	gl_Position = ubo.modelViewProj * vec4(inPosition, 1.0);
}
