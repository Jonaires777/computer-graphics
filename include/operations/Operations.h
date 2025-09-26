#pragma once
#include <glm/glm.hpp>

namespace Operations {
	inline glm::vec3 compMul(const glm::vec3& a, const glm::vec3& b) {
		return glm::vec3(a.x * b.x, a.y * b.y, a.z * b.z);
	}

	// if necessary
	inline glm::vec4 compMul(const glm::vec4& a, const glm::vec4& b) {
		return glm::vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
	}
}