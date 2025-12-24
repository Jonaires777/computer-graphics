#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Operations {
	inline glm::vec3 compMul(const glm::vec3& a, const glm::vec3& b) {
		return glm::vec3(a.x * b.x, a.y * b.y, a.z * b.z);
	}

	// if necessary
	inline glm::vec4 compMul(const glm::vec4& a, const glm::vec4& b) {
		return glm::vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
	}

	inline glm::mat4 translate(const glm::vec3& t) {
		return glm::translate(glm::mat4(1.0f), t);
	}

	inline glm::mat4 scale(const glm::vec3& s) {
		return glm::scale(glm::mat4(1.0f), s);
	}

	inline glm::mat4 rotateX(float angleRad) {
		return glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(1, 0, 0));
	}

	inline glm::mat4 rotateY(float angleRad) {
		return glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(0, 1, 0));
	}

	inline glm::mat4 rotateZ(float angleRad) {
		return glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(0, 0, 1));
	}

	inline glm::mat4 shear(float xy, float xz, float yx, float yz, float zx, float zy) {
		glm::mat4 m(1.0f);
		m[1][0] = xy;
		m[2][0] = xz;
		m[0][1] = yx;
		m[2][1] = yz;
		m[0][2] = zx;
		m[1][2] = zy;
		return m;
	}

	inline Ray transformRay(const Ray& r, const glm::mat4& M)
	{
		Point newOrigin;
		newOrigin.position = M * r.origin.position;

		glm::vec4 newDir = M * r.direction;
		newDir.w = 0.0f;

		return Ray(newOrigin, newDir);
	}
}