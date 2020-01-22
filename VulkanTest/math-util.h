#pragma once

#include <limits>

#include <glm/glm.hpp>

namespace hvk
{
	namespace util
	{
		namespace math
		{
			const float PI = 3.14159265358979323846f;
			const float HALF_PI = 1.57079632679489661923;

			template <typename T>
			bool almost_equal(T lhs, T rhs)
			{
				return std::abs(lhs - rhs) <= std::numeric_limits<T>::epsilon();
			}
			
			glm::vec3 getEulerAnglesFromMatrix(const glm::mat4& transform)
			{
				glm::vec3 angles(0.f);

				auto sinYaw = transform[2][0];
				//if (!(fabsf(sinYaw) <= 1.f))
				if (!almost_equal(fabsf(sinYaw), 1.f))
				{
					auto arcsin = -asinf(sinYaw);
					angles.y = arcsin;
					float c = 1.f / cosf(angles.y);
					angles.x = atan2f(transform[2][1] * c, transform[2][2] * c);
					angles.z = atan2f(transform[1][0] * c, transform[0][0] * c);
				}
				else
				{
					angles.z = 0.f;
					if (sinYaw < 0.f)
					{
						angles.y = HALF_PI;
						angles.x = atan2f(transform[0][1], transform[0][2]);
					}
					else
					{
						angles.y = -HALF_PI;
						angles.x = atan2f(-transform[0][1], -transform[0][2]);
					}
				}

				return angles;
			}
		}
	}
}
