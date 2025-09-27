#pragma once
#include <glm/glm.hpp>
#include "model/Ray.h"
#include "model/LightSource.h"
#include "model/Point.h"
#include "model/Objects/Sphere.h"

struct Plane
{
	Point point_pi;
	glm::vec4 normal_n;

	glm::vec3 K_ambient;   // Ka
	glm::vec3 K_diffuse;   // Kd
	glm::vec3 K_specular;  // Ks
	float shininess;       // m

	Plane(const Point& pi = {}, const glm::vec4& n = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))
		: point_pi(pi), normal_n(glm::normalize(n)),
		K_ambient(glm::vec3(0.1f)), K_diffuse(glm::vec3(0.6f, 0.6f, 0.6f)),
		K_specular(glm::vec3(1.0f)), shininess(32.0f)
	{
	}

	bool intersect(const Ray& ray, float& t_out) const;

	bool shade(const glm::vec3& Pi, const Ray& ray, const LightSource& light, const glm::vec3& I_A, const Sphere& sphere, glm::vec3& outColor) const;
};