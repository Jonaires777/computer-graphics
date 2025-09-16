#pragma once
#include <glm/glm.hpp>
#include "model/Point.h"
#include "model/Ray.h"

struct Sphere
{
	Point center;
	float radius;

	Sphere(const Point& center, float radius) {
		this->center = center;
		this->radius = radius;
	}

	bool intersects(const Ray& ray) const {
		glm::vec4 w = ray.origin.position - center.position;
		float b = 2.0f * glm::dot(glm::vec3(w), glm::vec3(ray.direction));
		float c = glm::dot(glm::vec3(w), glm::vec3(w)) - radius * radius;
		float discriminant = b * b - 4.0f * c;
		return discriminant >= 0.0f;
	}
};