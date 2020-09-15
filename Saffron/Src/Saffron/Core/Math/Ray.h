#pragma once

#include "Saffron/Core/Math/AABB.h"
#include "Saffron/System/SaffronMath.h"

namespace Se
{
struct Ray
{
	glm::vec3 Origin{}, Direction{};

	Ray(const glm::vec3 &origin, const glm::vec3 &direction)
	{
		Origin = origin;
		Direction = direction;
	}

	static Ray Zero()
	{
		return { {0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };
	}

	bool IntersectsAABB(const AABB &aabb, float &t) const
	{
		glm::vec3 dirfrac;
		// r.dir is unit direction vector of ray
		dirfrac.x = 1.0f / Direction.x;
		dirfrac.y = 1.0f / Direction.y;
		dirfrac.z = 1.0f / Direction.z;
		// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
		// r.org is origin of ray
		const glm::vec3 &lb = aabb.Min;
		const glm::vec3 &rt = aabb.Max;
		const float t1 = (lb.x - Origin.x) * dirfrac.x;
		const float t2 = (rt.x - Origin.x) * dirfrac.x;
		const float t3 = (lb.y - Origin.y) * dirfrac.y;
		const float t4 = (rt.y - Origin.y) * dirfrac.y;
		const float t5 = (lb.z - Origin.z) * dirfrac.z;
		const float t6 = (rt.z - Origin.z) * dirfrac.z;

		const auto tmin = glm::max<float>(glm::max<float>(glm::min<float>(t1, t2), glm::min<float>(t3, t4)), glm::min<float>(t5, t6));
		const auto tmax = glm::min<float>(glm::min<float>(glm::max<float>(t1, t2), glm::max<float>(t3, t4)), glm::max<float>(t5, t6));

		// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
		if ( tmax < 0 )
		{
			t = tmax;
			return false;
		}

		// if tmin > tmax, ray doesn't intersect AABB
		if ( tmin > tmax )
		{
			t = tmax;
			return false;
		}

		t = tmin;
		return true;
	}

	bool IntersectsTriangle(const glm::vec3 &A, const glm::vec3 &B, const glm::vec3 &C, float &t) const
	{
		const glm::vec3 E1 = B - A;
		const glm::vec3 E2 = C - A;
		const glm::vec3 N = cross(E1, E2);
		const float det = -glm::dot(Direction, N);
		const float invdet = 1.0 / det;
		const glm::vec3 AO = Origin - A;
		const glm::vec3 DAO = glm::cross(AO, Direction);
		const float u = glm::dot(E2, DAO) * invdet;
		const float v = -glm::dot(E1, DAO) * invdet;
		t = glm::dot(AO, N) * invdet;
		return (det >= 1e-6 && t >= 0.0 && u >= 0.0 && v >= 0.0 && (u + v) <= 1.0);
	}

};

}