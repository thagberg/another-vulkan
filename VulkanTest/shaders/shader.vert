#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 modelViewProj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

//vec2 positions[3] = vec2[](
    //vec2(0.0, -0.5),
    //vec2(0.5, 0.5),
    //vec2(-0.5, 0.5)
//);


void main() {
    //gl_Position = ubo.modelViewProj * vec4(inPosition, 1.0);
    gl_Position = ubo.modelViewProj * vec4(inPosition, 1.0);
    fragColor = inColor;
    //fragColor = vec3(1.0, 0.0, 0.0);
}