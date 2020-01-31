#version 450
#extension GL_ARB_separate_shader_objects : enable

struct LightColor {
	vec3 color;
	float intensity;
};

struct LightAttenuation {
	float constant;
	float linear;
	float quadratic;
};

struct SpotLight {
    vec3 direction;
    float umbra;
    float penumbra;
};

struct AmbientLight {
	LightColor lightColor;
};

struct DirectionalLight {
	vec3 direction;
	LightColor lightColor;
};

struct DynamicLight {
	float umbra;
	float penumbra;
	float intensity;
	float constant;
	float linear;
	float quadratic;
	vec3 pos;
	vec3 color;
	vec3 direction;

	//LightColor lightColor;
    //SpotLight spotlight;
	//LightAttenuation attenuation;
};

layout(location = 0) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in mat3 inTBN;

layout(std140, set = 0, binding = 0) uniform UniformLight {
	uint numLights;
	DynamicLight lights[10];
	AmbientLight ambient;
	DirectionalLight directional;
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
layout(set = 1, binding = 6) uniform sampler2D bdrfLutSampler;

layout (push_constant) uniform PushConstant {
	float gamma;
	bool sRGBTextures;
	float roughness;
	float metallic;
} push;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 9.0;

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


vec3 calculateDynamicRadiance(
	vec3 lightDir, 
	vec3 lightRadiance, 
	vec3 surfaceNormal, 
	vec3 viewDir, 
	vec3 F0,
	vec3 environmentColor,
	vec3 albedo,
	float roughness,
	float metallic) 
{

	vec3 lightReflect = reflect(-lightDir, surfaceNormal);
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
//		albedo.rgb = mix(albedo.rgb, environmentColor, 1.0 - roughness);
	//vec3 lambert = albedo / PI;
	vec3 lambert = albedo.rgb;
	return (kD * lambert + specular) * lightRadiance * NdotL;
}

float distanceFalloff(float distance, float constant, float linear, float quadratic)
{
	return 1.0 / (constant + linear * distance + quadratic * pow(distance, 2.0));	
}

float spotlightFalloff(float theta, float umbra, float penumbra)
{
    return clamp((theta - umbra) / (penumbra - umbra), 0.0, 1.0);
}

void main() {
    vec3 viewDir = normalize(ubo.cameraPos - fragPos);

    vec4 albedo = texture(diffuseSampler, fragTexCoord);
	vec3 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord).rgb;
	vec3 surfaceNormal = texture(normalSampler, fragTexCoord).rgb;
    surfaceNormal = normalize(surfaceNormal * 2.0 - 1.0);
    surfaceNormal = normalize(inTBN * surfaceNormal);

	float occlusion = metallicRoughness.r;
	float metallic = metallicRoughness.b * push.metallic;
	float roughness = metallicRoughness.g * push.roughness;
	float glossiness = 1.0 - roughness;
	int numSamples = max(int(16.0 * roughness), 1);

	//vec3 ambientLight = lbo.ambient.intensity * lbo.ambient.color;
	vec3 imageIrradiance = texture(irradianceSampler, surfaceNormal).rgb;
	vec3 ambientLight = lbo.ambient.lightColor.intensity * lbo.ambient.lightColor.color;

    vec3 environmentReflect = reflect(-viewDir, surfaceNormal);
    vec3 environmentColor = textureLod(environmentSampler, environmentReflect, roughness * MAX_REFLECTION_LOD).rgb;

	// surface reflection at zero incidence
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo.rgb, metallic);

	// calculate lighting from IBL
	float NdotV = max(dot(surfaceNormal, viewDir), 0.0);
	vec3 ikS = getSchlickFresnelRoughness(NdotV, F0, roughness);
	vec3 ikD = vec3(1.0) - ikS;
	ikD *= 1.0 - metallic;

	vec3 imageRadiance = vec3(0.0);

	vec2 envBRDF = texture(bdrfLutSampler, vec2(NdotV, roughness)).rg;
	vec3 iSpecular = environmentColor * (ikS * envBRDF.x + envBRDF.y);
	vec3 iDiffuse = imageIrradiance * albedo.rgb;
	vec3 iAmbient = ikD * iDiffuse + iSpecular; // TODO: multiply by ambient occlusion
	imageRadiance += iAmbient;
    
	// calculate lighting from analytic light sources
    vec3 dynamicRadiance = vec3(0.0);
    for (int i = 0; i < lbo.numLights; i++)
    {
        DynamicLight thisLight = lbo.lights[i];
        
        vec3 lightDir = fragPos - thisLight.pos;
		float lightDistance = length(lightDir);
        lightDir = -normalize(lightDir);
        float spotlightIntensity = 1.0;
        if (thisLight.umbra > 0.0)
        {
            float spotlightTheta = dot(lightDir, normalize(thisLight.direction));
            spotlightIntensity = spotlightFalloff(spotlightTheta, thisLight.umbra, thisLight.penumbra);
        }
        vec3 lightRadiance = thisLight.color * thisLight.intensity * spotlightIntensity * distanceFalloff(lightDistance, thisLight.constant, thisLight.linear, thisLight.quadratic);

		dynamicRadiance += calculateDynamicRadiance(
			lightDir, 
			lightRadiance, 
			surfaceNormal, 
			viewDir, 
			F0, 
			environmentColor, 
			albedo.rgb, 
			roughness, 
			metallic);
    }

	DirectionalLight directional = lbo.directional;
	vec3 lightRadiance = directional.lightColor.color * directional.lightColor.intensity;
	dynamicRadiance += calculateDynamicRadiance(
		-normalize(directional.direction),
		lightRadiance,
		surfaceNormal,
		viewDir,
		F0,
		environmentColor,
		albedo.rgb,
		roughness,
		metallic);

    vec4 ambientColor = albedo * vec4(ambientLight, 1.0);
    outColor = occlusion * ambientColor + vec4(imageRadiance, 0.0) + vec4(dynamicRadiance, 0.0);
}
