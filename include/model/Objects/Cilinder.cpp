#include "model/Objects/Cilinder.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>

Cilinder::Cilinder(
	const Point& baseCenter,
	float radius,
	float height,
	const glm::vec4& direction,
	bool hasBase,
	bool hasTop,
	const glm::vec3& Ka,
	const glm::vec3& Kd,
	const glm::vec3& Ks,
	float shin,
	const glm::vec3& Ka_caps,
	const glm::vec3& Kd_caps,
	const glm::vec3& Ks_caps,
	float shin_caps
)
	: 
	baseCenter(baseCenter), 
	radius(radius), 
	height(height), 
	direction(glm::normalize(direction)), 
	hasBase(hasBase), 
	hasTop(hasTop),
	K_ambient_caps(Ka_caps),
	K_diffuse_caps(Kd_caps),
	K_specular_caps(Ks_caps),
	shininess_caps(shin_caps)
{
	K_ambient = Ka;
	K_diffuse = Kd;
	K_specular = Ks;
	shininess = shin;
}

bool Cilinder::intersect(const Ray& ray, float& t_out) const
{
	glm::vec3 dc = glm::vec3(glm::normalize(direction));
	glm::vec3 dr = glm::normalize(glm::vec3(ray.direction));
	glm::vec3 w = glm::vec3(ray.origin.position) - glm::vec3(baseCenter.position);

	// Using the equivalent vetorial formula as M = I - (dc * dcT)
	float a = glm::dot(dr, dr) - glm::pow(glm::dot(dr, dc), 2);
	float b = 2.0f * (glm::dot(dr, w) - glm::dot(dr, dc) * glm::dot(w, dc));
	float c = glm::dot(w, w) - glm::pow(glm::dot(w, dc), 2) - radius * radius;

	float delta = b * b - 4 * a * c;
	float t_cylinder = std::numeric_limits<float>::infinity();

	if (delta >= 0.0f) {
		float sqrtDelta = std::sqrt(delta);
		float t1 = (-b - sqrtDelta) / (2.0f * a);
		float t2 = (-b + sqrtDelta) / (2.0f * a);

		if (t1 > 0.0f) t_cylinder = t1;
		if (t2 > 0.0f && t2 < t_cylinder) t_cylinder = t2;

		if (t_cylinder < std::numeric_limits<float>::infinity()) {
			glm::vec3 Pi = glm::vec3(ray.origin.position) + t_cylinder * dr;
			float proj = glm::dot(Pi - glm::vec3(baseCenter.position), dc);
			if (proj < 0.0f || proj > height)
				t_cylinder = std::numeric_limits<float>::infinity();
		}
	}

	float t_base = std::numeric_limits<float>::infinity();
	float t_top = std::numeric_limits<float>::infinity();

	if (hasBase) {
		float denom = glm::dot(dr, dc);
		if (std::abs(denom) > 1e-6f) {
			float t = glm::dot(glm::vec3(baseCenter.position) - glm::vec3(ray.origin.position), dc) / denom;
			if (t > 0.0f) {
				glm::vec3 Pi = glm::vec3(ray.origin.position) + t * dr;
				float dist = glm::length(Pi - glm::vec3(baseCenter.position));
				if (dist <= radius)
					t_base = t;
			}
		}
	}

	if (hasTop) {
		glm::vec3 topCenter = glm::vec3(baseCenter.position) + height * dc;
		float denom = glm::dot(dr, dc);
		if (std::abs(denom) > 1e-6f) {
			float t = glm::dot(topCenter - glm::vec3(ray.origin.position), dc) / denom;
			if (t > 0.0f) {
				glm::vec3 Pi = glm::vec3(ray.origin.position) + t * dr;
				float dist = glm::length(Pi - topCenter);
				if (dist <= radius)
					t_top = t;
			}
		}
	}

	t_out = t_cylinder;
	lastHitPart = 0;

	if (t_base < t_out) {
		t_out = t_base;
		lastHitPart = 1;
	}
	if (t_top < t_out) {
		t_out = t_top;
		lastHitPart = 2;
	}

	if (t_out == std::numeric_limits<float>::infinity())
		return false;

	return true;

}

glm::vec3 Cilinder::getNormal(const glm::vec3& Pi) const
{
	glm::vec3 dc = glm::normalize(glm::vec3(direction));
	glm::vec3 pc = glm::vec3(baseCenter.position);
	glm::vec3 topCenter = pc + height * dc;

	if (lastHitPart == 1) return -dc;
	if (lastHitPart == 2) return dc; 

	glm::vec3 v = Pi - pc;
	glm::vec3 proj = glm::dot(v, dc) * dc;
	glm::vec3 normal = glm::normalize(v - proj);

	if (!hasBase && glm::dot(normal, glm::normalize(-dc)) < 0.0f)
		normal = -normal;

	return normal;
}


