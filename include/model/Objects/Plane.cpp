#include "model/Objects/Plane.h"
#include "operations/Operations.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>

using namespace Operations;

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
    Ray localRay = transformRay(ray, invModel);
    return intersectLocal(localRay, t_out);
}

bool Plane::intersectLocal(const Ray& ray, float& t_out) const {
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

glm::vec3 Plane::getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const
{
    // -------- NORMAL LOCAL --------
    glm::vec3 normalLocal = glm::normalize(glm::vec3(normal_n));

    // -------- TRANSFORMA PARA WORLD --------
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    glm::vec3 normalWorld = glm::normalize(normalMatrix * normalLocal);

    // -------- GARANTE ORIENTAÇÃO --------
    if (glm::dot(normalWorld, rayDir) > 0.0f)
        normalWorld = -normalWorld;

    return normalWorld;
}

AABB Plane::getAABB() const {
    AABB box;
    float inf = std::numeric_limits<float>::infinity();
    box.min = glm::vec3(-inf);
    box.max = glm::vec3(inf);
    return box;
}