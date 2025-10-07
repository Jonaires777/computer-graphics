#include "model/Objects/Sphere.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>

Sphere::Sphere(
    const Point& c, 
    float r, 
    const glm::vec3& Ka,
    const glm::vec3& Kd,
    const glm::vec3& Ks,
    float shin
)
    : center(c), radius(r)
{
    K_ambient = Ka;
    K_diffuse = Kd;
    K_specular = Ks;
    shininess = shin;
}

bool Sphere::intersect(const Ray& ray, float& t_out) const {
    glm::vec3 O = glm::vec3(ray.origin.position);
    glm::vec3 D = glm::vec3(ray.direction);
    D = glm::normalize(D);

    glm::vec3 C = glm::vec3(center.position);
    glm::vec3 oc = O - C;
    float a = glm::dot(D, D);
    float b = 2.0f * glm::dot(oc, D);
    float c = glm::dot(oc, oc) - radius * radius;

    float disc = b * b - 4 * a * c;
    if (disc < 0.0f) {
        return false;
    }

    float sqrtDisc = std::sqrt(disc);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);

    t_out = (t1 > 1e-4f) ? t1 : ((t2 > 1e-4f) ? t2 : -1.0f);
    return t_out > 0;
}

glm::vec3 Sphere::getNormal(const glm::vec3& Pi) const {
    return glm::normalize(Pi - glm::vec3(center.position));
}