#include "model/Objects/Cilinder.h"
#include "operations/Operations.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>

using namespace Operations;

Cilinder::Cilinder(
	const Point& baseCenter,
	float radius,
	float height,
	const glm::vec4& direction,
	bool hasBase,
	bool hasTop,
	const glm::vec3& Ka,
	const glm::vec3& Kd,
	const glm::vec3& Ks,
	float shin,
	const glm::vec3& Ka_caps,
	const glm::vec3& Kd_caps,
	const glm::vec3& Ks_caps,
	float shin_caps
)
	: 
	baseCenter(baseCenter), 
	radius(radius), 
	height(height), 
	direction(glm::normalize(direction)), 
	hasBase(hasBase), 
	hasTop(hasTop),
	K_ambient_caps(Ka_caps),
	K_diffuse_caps(Kd_caps),
	K_specular_caps(Ks_caps),
	shininess_caps(shin_caps)
{
	K_ambient = Ka;
	K_diffuse = Kd;
	K_specular = Ks;
	shininess = shin;
}

bool Cilinder::intersect(const Ray& ray, float& t_out) const
{
    int dummy;
    return intersect(ray, t_out, dummy);
}

bool Cilinder::intersect(const Ray& ray, float& t_out, int& hitPart) const
{
    Ray localRay = transformRay(ray, invModel);
    return intersectLocal(localRay, t_out, hitPart);
}

bool Cilinder::intersectLocal(const Ray& ray, float& t_out, int& hitPart) const {
    glm::vec3 dc = glm::normalize(glm::vec3(direction));
    glm::vec3 dr = glm::normalize(glm::vec3(ray.direction));
    glm::vec3 w = glm::vec3(ray.origin.position) - glm::vec3(baseCenter.position);

    float a = glm::dot(dr, dr) - glm::pow(glm::dot(dr, dc), 2);
    float b = 2.0f * (glm::dot(dr, w) - glm::dot(dr, dc) * glm::dot(w, dc));
    float c = glm::dot(w, w) - glm::pow(glm::dot(w, dc), 2) - radius * radius;

    float delta = b * b - 4 * a * c;
    float t_cylinder = std::numeric_limits<float>::infinity();
    hitPart = 0; 

    if (delta >= 0.0f)
    {
        float sqrtDelta = std::sqrt(delta);
        float t1 = (-b - sqrtDelta) / (2.0f * a);
        float t2 = (-b + sqrtDelta) / (2.0f * a);

        for (float t : {t1, t2})
        {
            if (t > 1e-4f)
            {
                glm::vec3 Pi = glm::vec3(ray.origin.position) + t * dr;
                float proj = glm::dot(Pi - glm::vec3(baseCenter.position), dc);
                if (proj >= 0.0f && proj <= height)
                {
                    t_cylinder = t;
                    break;
                }
            }
        }
    }

    float t_base = std::numeric_limits<float>::infinity();
    float t_top = std::numeric_limits<float>::infinity();

    if (hasBase)
    {
        float denom = glm::dot(dr, dc);
        if (std::abs(denom) > 1e-6f)
        {
            float t = glm::dot(glm::vec3(baseCenter.position) - glm::vec3(ray.origin.position), dc) / denom;
            if (t > 1e-4f)
            {
                glm::vec3 Pi = glm::vec3(ray.origin.position) + t * dr;
                float dist = glm::length(Pi - glm::vec3(baseCenter.position));
                if (dist <= radius)
                    t_base = t;
            }
        }
    }

    if (hasTop)
    {
        glm::vec3 topCenter = glm::vec3(baseCenter.position) + height * dc;
        float denom = glm::dot(dr, dc);
        if (std::abs(denom) > 1e-6f)
        {
            float t = glm::dot(topCenter - glm::vec3(ray.origin.position), dc) / denom;
            if (t > 1e-4f)
            {
                glm::vec3 Pi = glm::vec3(ray.origin.position) + t * dr;
                float dist = glm::length(Pi - topCenter);
                if (dist <= radius)
                    t_top = t;
            }
        }
    }

    t_out = std::min({ t_cylinder, t_base, t_top });
    if (t_out == t_cylinder) hitPart = 0;
    else if (t_out == t_base) hitPart = 1;
    else if (t_out == t_top) hitPart = 2;

    return t_out < std::numeric_limits<float>::infinity();
}

glm::vec3 Cilinder::getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const
{
    // Esta versão recalcula qual parte foi atingida (menos eficiente mas thread-safe)
    glm::vec3 Pi_local = glm::vec3(invModel * glm::vec4(Pi, 1.0f));
    glm::vec3 dc = glm::normalize(glm::vec3(direction));
    glm::vec3 pc = glm::vec3(baseCenter.position);

    // Determina qual parte foi atingida baseado na posição
    int hitPart = 0;
    float proj = glm::dot(Pi_local - pc, dc);

    if (std::abs(proj) < 1e-4f && hasBase)
        hitPart = 1; // base
    else if (std::abs(proj - height) < 1e-4f && hasTop)
        hitPart = 2; // topo
    else
        hitPart = 0; // lateral

    return getNormal(Pi, rayDir, hitPart);
}

glm::vec3 Cilinder::getNormal(const glm::vec3& Pi, const glm::vec3& rayDir, int hitPart) const
{
    glm::vec3 Pi_local = glm::vec3(invModel * glm::vec4(Pi, 1.0f));

    glm::vec3 dc = glm::normalize(glm::vec3(direction));
    glm::vec3 pc = glm::vec3(baseCenter.position);

    glm::vec3 normalLocal;

    if (hitPart == 1)          // base
        normalLocal = -dc;
    else if (hitPart == 2)     // topo
        normalLocal = dc;
    else                       // lateral
    {
        glm::vec3 v = Pi_local - pc;
        glm::vec3 proj = glm::dot(v, dc) * dc;
        normalLocal = glm::normalize(v - proj);
    }

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    glm::vec3 normalWorld = glm::normalize(normalMatrix * normalLocal);

    if (glm::dot(normalWorld, rayDir) > 0.0f)
        normalWorld = -normalWorld;

    return normalWorld;
}

AABB Cilinder::getAABB() const {
    glm::vec3 posBase = glm::vec3(baseCenter.position);
    glm::vec3 posTop = posBase + glm::vec3(direction) * height;

    // Expandimos a caixa para conter as duas extremidades considerando o raio
    AABB box;
    box.min = glm::min(posBase, posTop) - glm::vec3(radius);
    box.max = glm::max(posBase, posTop) + glm::vec3(radius);
    return box;
}



