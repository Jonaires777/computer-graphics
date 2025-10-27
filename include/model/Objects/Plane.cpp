#include "model/Objects/Plane.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>

Plane::Plane(
    const Point& pi,
    const glm::vec4& n,
    const glm::vec3& Ka,
    const glm::vec3& Kd,
    const glm::vec3& Ks,
    float shin
)
    : point_pi(pi), normal_n(glm::normalize(n))
{
    K_ambient = Ka;
    K_diffuse = Kd;
    K_specular = Ks;
    shininess = shin;
}

bool Plane::intersect(const Ray& ray, float& t_out) const
{
    glm::vec3 O = glm::vec3(ray.origin.position);
    glm::vec3 D = glm::normalize(glm::vec3(ray.direction));

    glm::vec3 P0 = glm::vec3(point_pi.position);
    glm::vec3 n = glm::normalize(glm::vec3(normal_n));

    float denom = glm::dot(D, n);
    if (fabs(denom) < 1e-6f)
        return false;

    t_out = glm::dot(P0 - O, n) / denom;
    return t_out > 1e-4f;
}

glm::vec3 Plane::getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const {
    glm::vec3 normal = glm::normalize(glm::vec3(normal_n));
    if (glm::dot(normal, rayDir) > 0.0f)
        normal = -normal;
    return normal;
}