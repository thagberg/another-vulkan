#include "math-util.h"
#include <limits>
#include <glm/glm.hpp>

namespace hvk
{
    namespace util
    {
        namespace math
        {
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

            bool rayPlaneIntersection(const Ray& r, const Plane& p, glm::vec3& intersectsAt)
            {
                /*
                    t = ((p0 - l0) * n) / (l * n)
                */
                bool intersects = false;
                float denom = glm::dot(r.direction, p.normal);
                if (gt_zero(denom))
                {
                    float t = glm::dot(p.position - r.origin, p.normal) / denom;
                    if (t >= 0)
                    {
                        intersectsAt = t * r.direction + r.origin;
                        intersects = true;
                    }
                }

                return intersects;
            }

			glm::vec4 screenToClip(const glm::vec2& screenCoord, const glm::vec2& screenDimensions)
			{
				glm::vec4 clipCoord;

				auto ndc = screenCoord / screenDimensions;
				ndc.y = 1.f - ndc.y;
				ndc = ndc * 2.f - 1.f;
				clipCoord = glm::vec4(ndc.x, ndc.y, -1.f, 1.f);

				return clipCoord;
			}

			glm::vec4 clipToView(const glm::vec4& clipCoord, const glm::mat4& inverseProjection)
			{
				glm::vec4 view = inverseProjection * clipCoord;
				view = view / view.w;

				return view;
			}

			glm::vec4 screenToView(const glm::vec2& screenCoord, const glm::vec2& screenDimensions, const glm::mat4& inverseProjection)
			{
				auto clip = screenToClip(screenCoord, screenDimensions);
				return clipToView(clip, inverseProjection);
			}
        }
    }
}