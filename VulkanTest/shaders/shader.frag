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
layout(set = 1, binding = 5) uniform samplerCube irradianceSampler;

layout (push_constant) uniform PushConstant {
	float gamma;
	bool sRGBTextures;
	float roughness;
	float metallic;
} push;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 getSchlickFresnel(float dotP, vec3 F0) {
	return F0 + (1.0f - F0) * pow(1.0 - dotP, 5.0);
}

vec3 getSchlickFresnelRoughness(float dotP, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(1.0 - dotP, 5.0);
}

/*
	Trowbridge-Reitz GGX

	Approximates the general alignment of microfacets towards the half-vec

	a^2 / PI( (n * h)^2 * (a^2 - 1) + 1 ) ^ 2
	Where:
		a = roughness
		n = surfaceNormal
		h = halfVec between view and light directions
*/
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

/*
	Schlick-GGX

	Approximates the amount of self-shadowing on a surface based
	on its roughness and the angle of incident (view or light)

	n * v / (n * v) ( 1- k) + k
	
	0.0 == complete microfacet shadowing
	1.0 == no microfacet shadowing
*/
float getSchlickGeometryGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

/*
	Approximate self-shadowing based on light angle and view angle

	0.0 == complete microfacet shadowing
	1.0 == no microfacet shadowing
*/
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

	float metallic = metallicRoughness.b * push.metallic;
	float roughness = metallicRoughness.g * push.roughness;
	float glossiness = 1.0 - roughness;
	int numSamples = max(int(16.0 * roughness), 1);

	//vec3 ambientLight = lbo.ambient.intensity * lbo.ambient.color;
	vec3 imageIrradiance = texture(irradianceSampler, surfaceNormal).rgb;
	vec3 ambientLight = lbo.ambient.intensity * lbo.ambient.color;

    vec3 environmentReflect = reflect(viewDir, surfaceNormal);
    vec3 environmentColor = texture(environmentSampler, environmentReflect).rgb;

	// surface reflection at zero incidence
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo.rgb, metallic);

	// calculate lighting from IBL
	vec3 imageRadiance = vec3(0.0);
	vec3 ikS = getSchlickFresnelRoughness(max(dot(surfaceNormal, viewDir), 0.0), F0, roughness);
	vec3 ikD = vec3(1.0) - ikS;
	vec3 iDiffuse = imageIrradiance * albedo.rgb;
	vec3 iAmbient = ikD * iDiffuse; // TODO: multiply by ambient occlusion
	imageRadiance += iAmbient;
    
	// calculate lighting from analytic light sources
    vec3 dynamicRadiance = vec3(0.0);
    for (int i = 0; i < lbo.numLights; i++)
    {
        Light thisLight = lbo.lights[i];
        
        vec3 lightDir = normalize(thisLight.pos - fragPos);
        vec3 lightReflect = reflect(-lightDir, surfaceNormal);
        vec3 lightRadiance = thisLight.color * thisLight.intensity;
		vec3 halfVec = normalize(lightDir + viewDir);

        vec3 F = getSchlickFresnel(max(dot(halfVec, viewDir), 0.0), F0);

        float NDF = getDistributionGGX(surfaceNormal, halfVec, roughness);
        float G = GeometrySmith(surfaceNormal, viewDir, lightDir, roughness);

        vec3 numerator = NDF * G * F;
		// TODO: this formula is part of Cook-Torrance, but I don't actually understand the justification
		//	should eventually read some papers about that...
        float denominator = 4.0 * max(dot(surfaceNormal, viewDir), 0.0) * max(dot(surfaceNormal, lightDir), 0.0);
        vec3 specular = numerator / max(denominator, 0.001);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(surfaceNormal, lightDir), 0.0);
//		albedo.rgb = mix(albedo.rgb, environmentColor, glossiness);
		//vec3 lambert = albedo.rgb / PI;
		vec3 lambert = albedo.rgb;
        dynamicRadiance += (kD * lambert + specular) * lightRadiance * NdotL;
    }

    vec4 ambientColor = albedo * vec4(ambientLight, 1.0);
    outColor = ambientColor + vec4(imageRadiance, 0.0) + vec4(dynamicRadiance, 0.0);
}