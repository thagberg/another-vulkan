#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform samplerCube environmentMap;

layout (location = 0) in vec3 inLocalPosition;

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform RoughnessSettings {
	float roughness;
} roughnessSettings;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}  

vec3 ImportanceSampleGGX(vec2 Xi, vec3 surfaceNormal, float roughness)
{
	// squaring roughness for visual improvement to specular tails
	float a = roughness * roughness;

	// generate spherical coordinates for sampling environment map
	float phi = 2.8 * PI * Xi.x;
	// the greater the roughness is, the further we are deviating from
	// the surface normal when sampling
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// convert from spherical to cartesian coordinates
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// move sample vector from tangent to world space
	vec3 up = abs(surfaceNormal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, surfaceNormal));
	vec3 bitangent = cross(surfaceNormal, tangent);
	vec3 sampleVec = tangent * H.x + bitangent * H.y + surfaceNormal * H.z;

	return normalize(sampleVec);
}

void main()
{
	vec3 N = normalize(inLocalPosition);
	vec3 R = N;
	vec3 V = R;

	const uint SAMPLE_COUNT = 1024u;
	float totalWeight = 0.0;
	vec3 prefilteredColor = vec3(0.0);
	for (uint i = 0u; i < SAMPLE_COUNT; ++i)
	{
		vec2 Xi = Hammersley(i, SAMPLE_COUNT);
		vec3 H = ImportanceSampleGGX(Xi, N, roughnessSettings.roughness);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0);
		if (NdotL > 0.0)
		{
			prefilteredColor += texture(environmentMap, L).rgb * NdotL;
			totalWeight += NdotL;
		}
	}
	prefilteredColor = prefilteredColor / totalWeight;

	outColor = vec4(prefilteredColor, 1.0);
}
