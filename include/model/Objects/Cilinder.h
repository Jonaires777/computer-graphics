#pragma once
#include "model/Objects/Object.h"
#include "model/Ray.h"
#include "model/Point.h"
#include <glm/glm.hpp>

struct Cilinder : public Object
{
	Point baseCenter;
	float radius;
	float height;
	glm::vec4 direction;
	bool hasBase;
	bool hasTop;

	glm::vec3 K_ambient_caps;
	glm::vec3 K_diffuse_caps;
	glm::vec3 K_specular_caps;
	float shininess_caps;

	mutable int lastHitPart = 0;

	Cilinder(
		const Point& baseCenter = {},
		float radius = 1.0f,
		float height = 1.0f,
		const glm::vec4& direction = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
		bool hasBase = true,
		bool hasTop = true,
		const glm::vec3& Ka = glm::vec3(0.1f),
		const glm::vec3& Kd = glm::vec3(0.6f),
		const glm::vec3& Ks = glm::vec3(1.0f),
		float shin = 32.0f,
		const glm::vec3& Ka_caps = glm::vec3(0.1f),
		const glm::vec3& Kd_caps = glm::vec3(0.6f),
		const glm::vec3& Ks_caps = glm::vec3(1.0f),
		float shin_caps = 32.0f
	);

	bool intersect(const Ray& ray, float& t_out) const override;
	glm::vec3 getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const override;

private:
	bool intersectLocal(const Ray& localRay, float& t_out) const;
};