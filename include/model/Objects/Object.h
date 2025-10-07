#pragma once
#include <glm/glm.hpp>

struct Object
{
	glm::vec3 K_ambient;   
	glm::vec3 K_diffuse;   
	glm::vec3 K_specular;  
	float shininess;

	Object()
		: K_ambient(glm::vec3(0.1f)), K_diffuse(glm::vec3(0.6f, 0.6f, 0.6f)),
		K_specular(glm::vec3(1.0f)), shininess(32.0f)
	{
	}

	virtual bool intersect(const struct Ray& ray, float& t_out) const = 0;
	virtual glm::vec3 getNormal(const glm::vec3& point) const = 0;
	virtual ~Object() = default;
};
