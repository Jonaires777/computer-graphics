#pragma once
#include <glm/glm.hpp>
#include "model/Point.h"
#include "model/LightSource.h"
#include "model/Ray.h"

struct Sphere
{
	Point center;
	float radius;

	glm::vec3 K_ambient;   // Ka
	glm::vec3 K_diffuse;   // Kd
	glm::vec3 K_specular;  // Ks
	float shininess;       // m

	Sphere(const Point& c = {}, float r = 1.0f)
		: center(c), radius(r),
		K_ambient(glm::vec3(0.1f)), K_diffuse(glm::vec3(1.0f, 0.0f, 0.0f)),
		K_specular(glm::vec3(1.0f)), shininess(32.0f)
	{}

	bool shade(const Ray& ray, const LightSource& light, glm::vec3& outColor) const;
};