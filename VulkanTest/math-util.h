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

            struct Plane
            {
                glm::vec3 position;
                glm::vec3 normal;
            };

            struct Ray
            {
                glm::vec3 origin;
                glm::vec3 direction;
            };

            struct AABB
            {
                glm::vec3 min;
                glm::vec3 max;
            };

			template <typename T>
			bool almost_equal(T lhs, T rhs)
			{
				return std::abs(lhs - rhs) <= std::numeric_limits<T>::epsilon();
			}

            template <typename T>
            bool gt_zero(T v)
            {
                return std::abs(v - 0) > std::numeric_limits<T>::epsilon();
            }
			
            glm::vec3 getEulerAnglesFromMatrix(const glm::mat4& transform);

            bool rayPlaneIntersection(const Ray& r, const Plane& p, glm::vec3& intersectsAt);
		}
	}
}
