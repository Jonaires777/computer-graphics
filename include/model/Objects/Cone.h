#pragma once
#include "model/Objects/Object.h"
#include "model/Ray.h"
#include "model/Point.h"
#include <glm/glm.hpp>
#include "model/AABB.h"

struct Cone : public Object
{
	Point baseCenter;
	glm::vec4 direction;
	float height;
	float baseRadius;
	bool hasBase;

	glm::vec3 K_ambient_base;
	glm::vec3 K_diffuse_base;
	glm::vec3 K_specular_base;
	float shininess_base;

	mutable int lastHitPart = 0;

	Cone(
		const Point& baseCenter = {},
		const glm::vec4& direction = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
		float height = 1.0f,
		float baseRadius = 1.0f,
		bool hasBase = true,
		const glm::vec3& Ka = glm::vec3(0.1f),
		const glm::vec3& Kd = glm::vec3(0.6f),
		const glm::vec3& Ks = glm::vec3(1.0f),
		float shin = 32.0f,
		const glm::vec3& Ka_base = glm::vec3(0.1f),
		const glm::vec3& Kd_base = glm::vec3(0.6f),
		const glm::vec3& Ks_base = glm::vec3(1.0f),
		float shin_base = 32.0f
	);

	Cone(const Point& baseCenter, const Point& vertex, float baseRadius, bool vertexAsApex, bool hasBase = true);

	bool intersect(const Ray& ray, float& t_out) const override;
	glm::vec3 getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const override;
	AABB getAABB() const override;

private:
	bool intersectLocal(const Ray& localRay, float& t_out) const;
};
