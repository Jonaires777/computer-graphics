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
	glm::vec3 dc = glm::normalize(glm::vec3(direction));
	glm::vec3 ro = glm::vec3(ray.origin.position);
	glm::vec3 rd = glm::normalize(glm::vec3(ray.direction));

	// Define ápice do cone
	glm::vec3 apex = glm::vec3(baseCenter.position) + dc * height;

	// Vetores auxiliares
	glm::vec3 co = ro - apex;

	float k = baseRadius / height;
	float k2 = k * k;

	float dc_rd = glm::dot(rd, dc);
	float dc_co = glm::dot(co, dc);

	float a = glm::dot(rd, rd) - (1 + k2) * dc_rd * dc_rd;
	float b = 2.0f * (glm::dot(rd, co) - (1 + k2) * dc_rd * dc_co);
	float c = glm::dot(co, co) - (1 + k2) * dc_co * dc_co;

	float delta = b * b - 4 * a * c;

	if (delta < 0.0f) return false;

	float sqrtDelta = std::sqrt(delta);
	float t1 = (-b - sqrtDelta) / (2.0f * a);
	float t2 = (-b + sqrtDelta) / (2.0f * a);

	float t_cone = std::numeric_limits<float>::infinity();

	const float epsilon = 1e-4f;

	if (t1 > epsilon) t_cone = t1;
	if (t2 > epsilon && t2 < t_cone) t_cone = t2;

	if (t_cone < std::numeric_limits<float>::infinity())
	{
		glm::vec3 Pi = ro + t_cone * rd;
		float h = glm::dot(Pi - glm::vec3(baseCenter.position), dc);

		if (h < -epsilon || h > height + epsilon)
			t_cone = std::numeric_limits<float>::infinity();
	}

	float t_base = std::numeric_limits<float>::infinity();
	lastHitPart = 0;

	if (hasBase)
	{
		float denom = glm::dot(rd, dc);
		if (std::abs(denom) > 1e-6f)
		{
			float t = glm::dot(glm::vec3(baseCenter.position) - ro, dc) / denom;
			if (t > epsilon)
			{
				glm::vec3 P = ro + t * rd;
				float dist = glm::length(P - glm::vec3(baseCenter.position));
				if (dist <= baseRadius)
				{
					t_base = t;
				}
			}
		}

		if (t_base < t_cone)
		{
			t_cone = t_base;
			lastHitPart = 1;
		}
	}

	if (t_cone == std::numeric_limits<float>::infinity())
		return false;

	t_out = t_cone;
	return true;
}


glm::vec3 Cone::getNormal(const glm::vec3& Pi, const glm::vec3& viewDir) const
{
	glm::vec3 dc = glm::normalize(glm::vec3(direction));
	glm::vec3 pc = glm::vec3(baseCenter.position);

	if (lastHitPart == 1)
		return -dc;

	glm::vec3 apex = pc + dc * height;
	glm::vec3 v = Pi - apex;
	glm::vec3 v_proj = glm::dot(v, dc) * dc;

	glm::vec3 n = glm::normalize((v_proj - v) * (height / baseRadius));

	if (glm::dot(n, viewDir) > 0.0f)
		n = -n;

	return n;
}
