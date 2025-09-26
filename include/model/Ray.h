#pragma once
#include <glm/glm.hpp>
#include "model/Point.h"

struct Ray
{
	Point origin;
	glm::vec4 direction;

	Ray(const Point& origin, const glm::vec4 direction) {
		this->origin = origin;
		this->direction = glm::normalize(direction);
	}

	Ray(const Point& origin, const Point& endPoint) {
		this->origin = origin;
		this->direction = glm::normalize(endPoint.position - origin.position);
	}
};