#pragma once
#include <glm/glm.hpp>

struct Point
{
	glm::vec4 position;

	Point(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f)
	{
		position = glm::vec4(x, y, z, w);
	}
};