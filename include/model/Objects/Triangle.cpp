#include "model/Objects/Triangle.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>

Triangle::Triangle(
	const Point& v0,
	const Point& v1,
	const Point& v2,
	const glm::vec3& Ka,
	const glm::vec3& Kd,
	const glm::vec3& Ks,
	float shin
)
	: v0(v0), v1(v1), v2(v2)
{
	K_ambient = Ka;
	K_diffuse = Kd;
	K_specular = Ks;
	shininess = shin;
}

bool Triangle::intersect(const Ray& ray, float& t_out) const {
	const float EPSILON = 1e-6f;

	glm::vec3 v0v1 = glm::vec3(v1.position) - glm::vec3(v0.position);
	glm::vec3 v0v2 = glm::vec3(v2.position) - glm::vec3(v0.position);
	glm::vec3 pvec = glm::cross(glm::vec3(ray.direction), v0v2);
	float det = glm::dot(v0v1, pvec);

	if (fabs(det) < EPSILON)
		return false;

	float invDet = 1.0f / det;

	glm::vec3 tvec = glm::vec3(ray.origin.position) - glm::vec3(v0.position);
	float u = glm::dot(tvec, pvec) * invDet;
	if (u < 0.0f || u > 1.0f)
		return false;

	glm::vec3 qvec = glm::cross(tvec, v0v1);
	float v = glm::dot(glm::vec3(ray.direction), qvec) * invDet;
	if (v < 0.0f || u + v > 1.0f)
		return false;

	float t = glm::dot(v0v2, qvec) * invDet;

	if (t > EPSILON) {
		t_out = t;
		return true;
	}

	return false;
}

glm::vec3 Triangle::getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const {
	glm::vec3 edge1 = glm::vec3(v1.position) - glm::vec3(v0.position);
	glm::vec3 edge2 = glm::vec3(v2.position) - glm::vec3(v0.position);
	glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
	if (glm::dot(normal, rayDir) > 0.0f)
		normal = -normal;
	return normal;
}