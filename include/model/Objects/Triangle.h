#pragma once
#include "model/Objects/Object.h"
#include "model/Ray.h"
#include "model/Point.h"
#include <glm/glm.hpp>
#include "model/AABB.h"

struct Triangle : public Object
{
	Point v0;
	Point v1;
	Point v2;
	glm::vec2 uv0, uv1, uv2;

	Triangle(
		const Point& v0 = {},
		const Point& v1 = {},
		const Point& v2 = {},
		const glm::vec2& uv0 = { 0,0 }, 
		const glm::vec2& uv1 = { 0,0 }, 
		const glm::vec2& uv2 = { 0,0 },
		const glm::vec3& Ka = glm::vec3(0.1f),
		const glm::vec3& Kd = glm::vec3(0.6f),
		const glm::vec3& Ks = glm::vec3(1.0f),
		float shin = 32.0f
	);

	bool intersect(const Ray& ray, float& t_out) const override;
	glm::vec3 getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const override;
	AABB getAABB() const override;

	bool intersectBaricentric(const Ray& ray, float& t_out, float& u_out, float& v_out) const;

private:
	bool intersectLocal(const Ray& localRay, float& t_out) const;
};
