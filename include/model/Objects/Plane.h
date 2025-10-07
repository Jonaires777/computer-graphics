#pragma once
#include "model/Objects/Object.h"
#include "model/Ray.h"
#include "model/Point.h"
#include <glm/glm.hpp>

struct Plane : public Object
{
    Point point_pi;
    glm::vec4 normal_n;

    Plane(
        const Point& pi = {},
        const glm::vec4& n = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
        const glm::vec3& Ka = glm::vec3(0.1f),
        const glm::vec3& Kd = glm::vec3(0.6f),
        const glm::vec3& Ks = glm::vec3(1.0f),
        float shin = 32.0f
    );

    bool intersect(const Ray& ray, float& t_out) const override;
    glm::vec3 getNormal(const glm::vec3& Pi) const override;
};
