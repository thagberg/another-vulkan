#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform samplerCube environmentMap;

layout(location = 0) in vec3 localPosition;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 convolution(vec3 normal)
{
	vec3 irradiance = vec3(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);
	const vec3 right = cross(up, normal);
	up = cross(normal, right);

	float sampleDelta = 0.025;
	int numSamples = 0;

	// We sweep "around" the hemisphere with phi, starting at the "equator"
	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
		// Then we sweep "upwards" towards the pole with theta
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
		{
			// convert spherical coordinates to cartesian coordinates
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

			// tangent space cartesian coordinates to world coordinates
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

			irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
			numSamples++;
		}
	}

	irradiance = PI * irradiance * (1.0 / float(numSamples));
	return irradiance;
}

void main()
{
	vec3 normal = normalize(localPosition);
	// account for coordinate system
	normal.y *= -1.0;
	normal.x *= -1.0;
	vec3 irradiance = convolution(normal);

	outColor = vec4(irradiance, 1.0);
}
