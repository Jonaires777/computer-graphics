#include "model/Objects/Cone.h"
#include "operations/Operations.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>

using namespace Operations;

Cone::Cone(
	const Point& baseCenter,
	const glm::vec4& direction,
	float height,
	float baseRadius,
	bool hasBase,
	const glm::vec3& Ka,
	const glm::vec3& Kd,
	const glm::vec3& Ks,
	float shin,
	const glm::vec3& Ka_base,
	const glm::vec3& Kd_base,
	const glm::vec3& Ks_base,
	float shin_base
)
	: baseCenter(baseCenter),
	direction(glm::normalize(direction)),
	height(height),
	baseRadius(baseRadius),
	hasBase(hasBase),
	K_ambient_base(Ka_base),
	K_diffuse_base(Kd_base),
	K_specular_base(Ks_base),
	shininess_base(shin_base)
{
	K_ambient = Ka;
	K_diffuse = Kd;
	K_specular = Ks;
	shininess = shin;
}

Cone::Cone(const Point& baseCenter, const Point& vertex, float baseRadius, bool vertexAsApex, bool hasBase)
	: baseRadius(baseRadius), hasBase(hasBase)
{
	if (vertexAsApex)
	{
		direction = glm::normalize(glm::vec4(glm::vec3(baseCenter.position) - glm::vec3(vertex.position), 0.0f));
		height = glm::length(glm::vec3(baseCenter.position) - glm::vec3(vertex.position));
		this->baseCenter = baseCenter;
	}
	else
	{
		direction = glm::normalize(glm::vec4(glm::vec3(vertex.position) - glm::vec3(baseCenter.position), 0.0f));
		height = glm::length(glm::vec3(vertex.position) - glm::vec3(baseCenter.position));
		this->baseCenter = baseCenter;
	}


	K_ambient = glm::vec3(0.1f);
	K_diffuse = glm::vec3(0.6f);
	K_specular = glm::vec3(1.0f);
	shininess = 32.0f;

	K_ambient_base = glm::vec3(0.1f);
	K_diffuse_base = glm::vec3(0.6f);
	K_specular_base = glm::vec3(1.0f);
	shininess_base = 32.0f;
}

bool Cone::intersect(const Ray& ray, float& t_out) const
{
	Ray localRay = transformRay(ray, invModel);
	return intersectLocal(localRay, t_out);
}

bool Cone::intersectLocal(const Ray& ray, float& t_out) const {
	glm::vec3 dc = glm::normalize(glm::vec3(direction));
	glm::vec3 ro = glm::vec3(ray.origin.position);
	glm::vec3 rd = glm::normalize(glm::vec3(ray.direction));

	// Define ápice do cone
	glm::vec3 apex = glm::vec3(baseCenter.position) + dc * height;

	// Vetores auxiliares
	glm::vec3 co = ro - apex;

	float k = baseRadius / height;
	float k2 = k * k;

	float dc_rd = glm::dot(rd, dc);
	float dc_co = glm::dot(co, dc);

	float a = glm::dot(rd, rd) - (1 + k2) * dc_rd * dc_rd;
	float b = 2.0f * (glm::dot(rd, co) - (1 + k2) * dc_rd * dc_co);
	float c = glm::dot(co, co) - (1 + k2) * dc_co * dc_co;

	float delta = b * b - 4 * a * c;

	if (delta < 0.0f) return false;

	float sqrtDelta = std::sqrt(delta);
	float t1 = (-b - sqrtDelta) / (2.0f * a);
	float t2 = (-b + sqrtDelta) / (2.0f * a);

	float t_cone = std::numeric_limits<float>::infinity();

	const float epsilon = 1e-4f;

	if (t1 > epsilon) t_cone = t1;
	if (t2 > epsilon && t2 < t_cone) t_cone = t2;

	if (t_cone < std::numeric_limits<float>::infinity())
	{
		glm::vec3 Pi = ro + t_cone * rd;
		float h = glm::dot(Pi - glm::vec3(baseCenter.position), dc);

		if (h < -epsilon || h > height + epsilon)
			t_cone = std::numeric_limits<float>::infinity();
	}

	float t_base = std::numeric_limits<float>::infinity();
	lastHitPart = 0;

	if (hasBase)
	{
		float denom = glm::dot(rd, dc);
		if (std::abs(denom) > 1e-6f)
		{
			float t = glm::dot(glm::vec3(baseCenter.position) - ro, dc) / denom;
			if (t > epsilon)
			{
				glm::vec3 P = ro + t * rd;
				float dist = glm::length(P - glm::vec3(baseCenter.position));
				if (dist <= baseRadius)
				{
					t_base = t;
				}
			}
		}

		if (t_base < t_cone)
		{
			t_cone = t_base;
			lastHitPart = 1;
		}
	}

	if (t_cone == std::numeric_limits<float>::infinity())
		return false;

	t_out = t_cone;
	return true;
}

glm::vec3 Cone::getNormal(const glm::vec3& Pi, const glm::vec3& viewDir) const
{
	glm::vec3 dc = glm::normalize(glm::vec3(direction));
	glm::vec3 base = glm::vec3(baseCenter.position);
	glm::vec3 apex = base + dc * height;

	const float eps = 1e-4f;
	float h = glm::dot(Pi - base, dc);

	glm::vec3 normalLocal;

	// BASE
	if (hasBase && h < eps)
	{
		normalLocal = -dc; // Aponta para baixo, fora da base
	}
	else
	{
		// LATERAL
		glm::vec3 v = Pi - apex;

		// A inclinação da lateral do cone (slope)
		float k = baseRadius / height;
		float m = k * k;

		// Cálculo da normal para a superfície lateral
		// Projeta o ponto no eixo e ajusta pela inclinação
		glm::vec3 n = (Pi - apex) - dc * (glm::dot(Pi - apex, dc) * (1.0f + m));

		// Se o cone ainda estiver escuro, tente inverter o sinal de 'n' aqui:
		normalLocal = glm::normalize(n);
	}

	// Transforma a normal para o espaço do mundo corretamente
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glm::vec3 normalWorld = glm::normalize(normalMatrix * normalLocal);

	return normalWorld;
}

AABB Cone::getAABB() const {
	glm::vec3 posBase = glm::vec3(baseCenter.position);
	glm::vec3 dirNorm = glm::normalize(glm::vec3(direction)); // Garante que é unitário
	glm::vec3 posApex = posBase + dirNorm * height;

	AABB box;
	// Fórmula: raio * sqrt(1.0 - direção^2)
	glm::vec3 extent;
	extent.x = baseRadius * sqrt(1.0f - dirNorm.x * dirNorm.x);
	extent.y = baseRadius * sqrt(1.0f - dirNorm.y * dirNorm.y);
	extent.z = baseRadius * sqrt(1.0f - dirNorm.z * dirNorm.z);

	// Caixa que envolve a Base Circular
	glm::vec3 baseMin = posBase - extent;
	glm::vec3 baseMax = posBase + extent;

	// A caixa final é a união da "Caixa da Base" com o "Ponto do Topo"
	box.min = glm::min(baseMin, posApex);
	box.max = glm::max(baseMax, posApex);

	// Margem de segurança (Epsilon) para evitar erros de precisão flutuante nas bordas
	box.min -= 0.001f;
	box.max += 0.001f;

	return box;
}