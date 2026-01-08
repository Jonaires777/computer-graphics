#pragma once
#include <glm/glm.hpp>
#include "model/AABB.h"

struct Object
{
	glm::vec3 K_ambient;   
	glm::vec3 K_diffuse;   
	glm::vec3 K_specular;  
	float shininess;
	
	glm::mat4 model = glm::mat4(1.0f);       
	glm::mat4 invModel = glm::mat4(1.0f);    
	glm::mat3 normalMatrix = glm::mat3(1.0f);

	Object()
		: K_ambient(glm::vec3(0.1f)), K_diffuse(glm::vec3(0.6f, 0.6f, 0.6f)),
		K_specular(glm::vec3(1.0f)), shininess(32.0f)
	{
	}

	virtual bool intersect(const struct Ray& ray, float& t_out) const = 0;
	virtual glm::vec3 getNormal(const glm::vec3& point, const glm::vec3& rayDir) const = 0;
	virtual ~Object() = default;
	virtual AABB getAABB() const = 0;

	void setTransform(const glm::mat4& M) {
		model = M;
		invModel = glm::inverse(M);
		normalMatrix = glm::transpose(glm::inverse(glm::mat3(M)));
	}
};
