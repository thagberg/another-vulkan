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
layout(set = 1, binding = 4) uniform samplerCube environmentSampler;

layout (push_constant) uniform PushConstant {
	float gamma;
	bool sRGBTextures;
} push;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 getSchlickFresnel(float dotP, vec3 F0) {
	return F0 + (1.0f - F0) * pow(1.0 - dotP, 5);
}

float getDistributionGGX(vec3 surfaceNormal, vec3 halfVec, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(surfaceNormal, halfVec), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / denom;
}

float getSchlickGeometryGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = getSchlickGeometryGGX(NdotV, roughness);
    float ggx1  = getSchlickGeometryGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

void main() {
    vec3 viewDir = normalize(ubo.cameraPos - fragPos);

    vec4 albedo = texture(diffuseSampler, fragTexCoord);
	vec3 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord).rgb;
	vec3 surfaceNormal = texture(normalSampler, fragTexCoord).rgb;
    surfaceNormal = normalize(surfaceNormal * 2.0 - 1.0);
    surfaceNormal = normalize(inTBN * surfaceNormal);

	vec3 ambientLight = lbo.ambient.intensity * lbo.ambient.color;

    vec3 environmentReflect = reflect(viewDir, surfaceNormal);
    vec3 environmentColor = texture(environmentSampler, environmentReflect).rgb;
    
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < lbo.numLights; i++)
    {
        Light thisLight = lbo.lights[i];
        
        vec3 lightDir = normalize(thisLight.pos - fragPos);
        vec3 lightReflect = reflect(-lightDir, surfaceNormal);
        vec3 lightRadiance = thisLight.color * thisLight.intensity;
		vec3 halfVec = normalize(lightDir + viewDir);

        // surface reflection at zero incidence
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, albedo.rgb, metallicRoughness.b);
        vec3 F = getSchlickFresnel(max(dot(halfVec, viewDir), 0.0), F0);

        float NDF = getDistributionGGX(surfaceNormal, halfVec, metallicRoughness.g);
        float G = GeometrySmith(surfaceNormal, viewDir, lightDir, metallicRoughness.g);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(surfaceNormal, viewDir), 0.0) * max(dot(surfaceNormal, lightDir), 0.0);
        vec3 specular = numerator / max(denominator, 0.001);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallicRoughness.b;

        float NdotL = max(dot(surfaceNormal, lightDir), 0.0);
        //Lo += (kD * albedo.rgb / PI + specular) * lightRadiance * NdotL;
        Lo += (kD * albedo.rgb / PI + specular) * mix(environmentColor, thisLight.color, metallicRoughness.g) * NdotL * thisLight.intensity;
        //vec3 dynamicColor = (kD * albedo.rgb / PI + specular) * lightRadiance * NdotL;
    }

    vec4 ambientColor = albedo * vec4(ambientLight, 1.0);
    outColor = ambientColor + vec4(Lo, 1.0);


    /*
	vec3 viewDir = normalize(ubo.cameraPos - fragPos);
	vec4 baseColor = texture(diffuseSampler, fragTexCoord);
    vec4 correctedColor = baseColor;
    if (push.sRGBTextures) {
        //correctedColor = vec4(pow(baseColor.rgb, vec3(push.gamma)), baseColor.a);
        correctedColor = vec4(pow(baseColor.rgb, vec3(2.2)), baseColor.a);
    }

	vec3 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord).rgb;
	vec3 surfaceNormal = texture(normalSampler, fragTexCoord).rgb;
    surfaceNormal = normalize(surfaceNormal * 2.0 - 1.0);
    surfaceNormal = normalize(inTBN * surfaceNormal);

    vec3 environmentReflect = reflect(viewDir, surfaceNormal);
    vec3 environmentColor = texture(environmentSampler, environmentReflect).rgb;

	vec3 ambientLight = lbo.ambient.intensity * lbo.ambient.color;
	vec3 diffuseLight = vec3(0.f, 0.f, 0.f);
	vec3 specularLight = vec3(0.f, 0.f, 0.f);

    vec3 testColor = vec3(0.f, 0.f, 0.f);

	vec4 dynamicColor = vec4(0.f, 0.f, 0.f, 0.f);
	for (int i = 0; i < lbo.numLights; i++) {
		Light thisLight = lbo.lights[i];
		vec3 lightDir = normalize(thisLight.pos - fragPos);
		vec3 reflectDir = reflect(-lightDir, surfaceNormal);
		vec3 halfVec = normalize(lightDir + viewDir);
		float d = thisLight.intensity * max(dot(surfaceNormal, lightDir), 0);
		float s = thisLight.intensity * pow(max(dot(viewDir, reflectDir), 0.f), 32);

        float reflectance = (1.0 - metallicRoughness.g);
        vec3 reflectColor = thisLight.color * environmentColor * metallicRoughness.b;
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, correctedColor.rgb, metallicRoughness.b);
		//specularLight += reflectColor * (1.0 - metallicRoughness.g) * getSchlickFresnel(max(dot(halfVec, viewDir), 0), F0) * s;
		specularLight += (1.0 - metallicRoughness.g) * getSchlickFresnel(max(dot(halfVec, viewDir), 0), F0) * s;
        testColor += getSchlickFresnel(max(dot(halfVec, viewDir), 0), F0);
        //specularLight = mix(specularLight, reflectColor, metallicRoughness.b);
        vec3 specularColor = thisLight.color;
        specularColor = specularLight * mix(specularColor, reflectColor, metallicRoughness.b);
		diffuseLight = (1.0 - specularLight) * thisLight.color * d;
		dynamicColor += vec4(diffuseLight, 1.f) * vec4(mix(reflectColor, correctedColor.rgb, reflectance), correctedColor.a);
		dynamicColor += vec4(specularLight, 1.f);
		//dynamicColor += vec4(specularColor, 1.f);

        //dynamicColor = vec4(reflectColor, 1.f);
	}
	vec4 ambientColor = vec4(ambientLight, 1.f) * correctedColor;

    // gamma correction
	vec4 fragColor = ambientColor + dynamicColor;
    fragColor.rgb = pow(fragColor.rgb, vec3(1.0/push.gamma));

	outColor = fragColor;
    //outColor = vec4(environmentColor, 1.0);
    //outColor = dynamicColor;
    //outColor = vec4(testColor, 1.0);
    //outColor = vec4(specularLight, 1.0);
    */
}