#pragma once
#include "model/Objects/Object.h"
#include "model/Ray.h"
#include "model/Point.h"
#include <glm/glm.hpp>
#include "model/AABB.h"

struct Sphere : public Object
{
	Point center;
	float radius;

    Sphere(
        const Point& c = {},
        float r = 1.0f,
        const glm::vec3& Ka = glm::vec3(0.1f),
        const glm::vec3& Kd = glm::vec3(0.6f),
        const glm::vec3& Ks = glm::vec3(1.0f),
        float shin = 32.0f
    );

	bool intersect(const Ray& ray, float& t_out) const override;
    glm::vec3 getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const override;
    AABB getAABB() const override;

private:
    bool intersectLocal(const Ray& localRay, float& t_out) const;
};