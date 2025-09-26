#pragma once
#include "model/Point.h"
#include <glm/glm.hpp>

struct LightSource {
	glm::vec3 intensity;
	Point position;

	LightSource(const glm::vec3& intensity, const Point& position) {
		this->intensity = intensity;
		this->position = position;
	}
};
