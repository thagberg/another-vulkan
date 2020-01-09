#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 modelViewProj;
	vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

//layout(location = 0) out vec3 fragColor;
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out mat3 outTBN;


void main() {
    gl_Position = ubo.modelViewProj * vec4(inPosition, 1.0);
    //gl_Position = vec4(inPosition, 1.0) * ubo.modelViewProj;
    //fragColor = inColor;
	fragTexCoord = inTexCoord;
	//outNormal = ubo.modelViewProj * vec4(inNormal, 1.0);
	outNormal = inNormal;
	fragPos = vec3(ubo.model * vec4(inPosition, 1.0));

    // calculate TBN
    vec3 T = normalize(vec3(ubo.model * vec4(inTangent.xyz, 0.0)));
    vec3 N = normalize(vec3(ubo.model * vec4(inNormal, 0.0)));
    vec3 B = cross(N, T) * -inTangent.w;
    outTBN = mat3(T, B, N);
}