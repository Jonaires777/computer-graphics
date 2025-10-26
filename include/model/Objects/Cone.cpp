#include "model/Objects/Cone.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>

Cone::Cone(
	const Point& baseCenter,
	const glm::vec4& direction,
	float height,
	float baseRadius,
	bool hasBase,
	const glm::vec3& Ka,
	const glm::vec3& Kd,
	const glm::vec3& Ks,
	float shin,
	const glm::vec3& Ka_base,
	const glm::vec3& Kd_base,
	const glm::vec3& Ks_base,
	float shin_base
)
	: baseCenter(baseCenter),
	direction(glm::normalize(direction)),
	height(height),
	baseRadius(baseRadius),
	hasBase(hasBase),
	K_ambient_base(Ka_base),
	K_diffuse_base(Kd_base),
	K_specular_base(Ks_base),
	shininess_base(shin_base)
{
	K_ambient = Ka;
	K_diffuse = Kd;
	K_specular = Ks;
	shininess = shin;
}

Cone::Cone(const Point& baseCenter, const Point& vertex, float baseRadius, bool vertexAsApex, bool hasBase)
	: baseRadius(baseRadius), hasBase(hasBase)
{
	if (vertexAsApex)
	{
		direction = glm::normalize(glm::vec4(glm::vec3(baseCenter.position) - glm::vec3(vertex.position), 0.0f));
		height = glm::length(glm::vec3(baseCenter.position) - glm::vec3(vertex.position));
		this->baseCenter = baseCenter;
	}
	else
	{
		direction = glm::normalize(glm::vec4(glm::vec3(vertex.position) - glm::vec3(baseCenter.position), 0.0f));
		height = glm::length(glm::vec3(vertex.position) - glm::vec3(baseCenter.position));
		this->baseCenter = baseCenter;
	}


	K_ambient = glm::vec3(0.1f);
	K_diffuse = glm::vec3(0.6f);
	K_specular = glm::vec3(1.0f);
	shininess = 32.0f;

	K_ambient_base = glm::vec3(0.1f);
	K_diffuse_base = glm::vec3(0.6f);
	K_specular_base = glm::vec3(1.0f);
	shininess_base = 32.0f;
}

bool Cone::intersect(const Ray& ray, float& t_out) const
{
	glm::vec3 dc = glm::vec3(glm::normalize(direction));
	glm::vec3 dr = glm::normalize(glm::vec3(ray.direction)); 
	glm::vec3 w = glm::vec3(ray.origin.position) - glm::vec3(baseCenter.position);

	float k = (height / baseRadius);
	k = k * k;

	float dr_dot_dc = glm::dot(dr, dc);
	float w_dot_dc = glm::dot(w, dc);

	float a = glm::dot(dr, dr) - (1 + k) * dr_dot_dc * dr_dot_dc;
	float b = 2 * (glm::dot(dr, w) - (1 + k) * dr_dot_dc * w_dot_dc + height * k * dr_dot_dc);
	float c = glm::dot(w, w) - (1 + k) * w_dot_dc * w_dot_dc + 2 * height * k * w_dot_dc - height * height * k;

	float delta = b * b - 4 * a * c;

	float t_cone = std::numeric_limits<float>::infinity();

	if (delta >= 0.0f)
	{
		float sqrtDelta = std::sqrt(delta);
		float t1 = (-b - sqrtDelta) / (2.0f * a);
		float t2 = (-b + sqrtDelta) / (2.0f * a);

		if (t1 > 0.0f) t_cone = t1;
		if (t2 > 0.0f && t2 < t_cone) t_cone = t2;

		if (t_cone < std::numeric_limits<float>::infinity())
		{
			glm::vec3 Pi = glm::vec3(ray.origin.position) + t_cone * dr;
			float proj = glm::dot(Pi - glm::vec3(baseCenter.position), dc);

			if (proj < 0.0f || proj > height)
				t_cone = std::numeric_limits<float>::infinity();
		}
	}

	float t_base = std::numeric_limits<float>::infinity();

	if (hasBase)
	{
		float denom = glm::dot(dr, dc);
		if (std::abs(denom) > 1e-6f)
		{
			float t = glm::dot(glm::vec3(baseCenter.position) - glm::vec3(ray.origin.position), dc) / denom;
			if (t > 0.0f)
			{
				glm::vec3 Pi = glm::vec3(ray.origin.position) + t * dr;
				float dist = glm::length(Pi - glm::vec3(baseCenter.position));
				if (dist <= baseRadius)
					t_base = t;
			}
		}
	}

	t_out = t_cone;
	lastHitPart = 0;

	if (t_base < t_out)
	{
		t_out = t_base;
		lastHitPart = 1;
	}

	if (t_out == std::numeric_limits<float>::infinity())
		return false;

	return true;
}

glm::vec3 Cone::getNormal(const glm::vec3& Pi) const
{
	glm::vec3 dc = glm::normalize(glm::vec3(direction));
	glm::vec3 pc = glm::vec3(baseCenter.position);

	if (lastHitPart == 1)
		return -dc;

	glm::vec3 apex = pc + dc * height;
	glm::vec3 v = Pi - apex;

	float proj = glm::dot(v, dc);
	glm::vec3 projPoint = apex + proj * dc;

	glm::vec3 normal = glm::normalize(Pi - projPoint);

	float sinTheta = baseRadius / glm::sqrt(height * height + baseRadius * baseRadius);
	glm::vec3 adjustedNormal = glm::normalize(normal * (1.0f - sinTheta));

	return adjustedNormal;
}
