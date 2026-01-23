#pragma once
#include <glm/glm.hpp>
#include <cmath>

namespace Operations {
	inline glm::vec3 compMul(const glm::vec3& a, const glm::vec3& b) {
		return glm::vec3(a.x * b.x, a.y * b.y, a.z * b.z);
	}

	inline glm::vec4 compMul(const glm::vec4& a, const glm::vec4& b) {
		return glm::vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
	}

	inline glm::mat4 translate(const glm::vec3& t) {
		glm::mat4 m(1.0f);
		m[3][0] = t.x;
		m[3][1] = t.y;
		m[3][2] = t.z;
		return m;
	}

	inline glm::mat4 scale(const glm::vec3& s) {
		glm::mat4 m(0.0f);
		m[0][0] = s.x;
		m[1][1] = s.y;
		m[2][2] = s.z;
		m[3][3] = 1.0f;
		return m;
	}

	inline glm::mat4 rotateX(float angleRad) {
		glm::mat4 m(1.0f);
		float c = std::cos(angleRad);
		float s = std::sin(angleRad);

		m[1][1] = c;
		m[1][2] = s;
		m[2][1] = -s;
		m[2][2] = c;

		return m;
	}

	inline glm::mat4 rotateY(float angleRad) {
		glm::mat4 m(1.0f);
		float c = std::cos(angleRad);
		float s = std::sin(angleRad);

		m[0][0] = c;
		m[0][2] = -s;
		m[2][0] = s;
		m[2][2] = c;

		return m;
	}

	inline glm::mat4 rotateZ(float angleRad) {
		glm::mat4 m(1.0f);
		float c = std::cos(angleRad);
		float s = std::sin(angleRad);

		m[0][0] = c;
		m[0][1] = s;
		m[1][0] = -s;
		m[1][1] = c;

		return m;
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
		// 1. Transformar a Origem (Ponto)
		// Assume-se que r.origin.position tem w=1 ou o GLM trata como ponto
		Point newOrigin;
		newOrigin.position = M * r.origin.position;

		// 2. Transformar a Direção (Vetor) -> CORREÇÃO AQUI
		glm::vec4 dir = r.direction;

		// OBRIGATÓRIO: Zerar o W para ignorar a translação da matriz
		dir.w = 0.0f;

		// Agora multiplicamos. A translação da matriz M será ignorada.
		glm::vec4 newDir = M * dir;

		return Ray(newOrigin, newDir);
	}
}