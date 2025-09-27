#include "model/Objects/Sphere.h"
#include "operations/Operations.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

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

bool Sphere::shade(const glm::vec3& Pi, const glm::vec3& n, const Ray& ray, const LightSource& light, glm::vec3& I_A, glm::vec3& outColor) const {
    using namespace Operations;

    // Vetores necessários para o cálculo de iluminação
    glm::vec3 Pf = glm::vec3(light.position.position);

    // Vetor da luz (l)
    glm::vec3 l = glm::normalize(Pf - Pi);

    // Vetor da visão (v)
    glm::vec3 D = glm::vec3(ray.direction);
    glm::vec3 v = glm::normalize(-D);

    // Vetor de reflexão (r)
    glm::vec3 r = glm::normalize(2 * glm::dot(n, l) * n - l);

    // Intensidades e coeficientes de refletividade
    glm::vec3 I_F = light.intensity;

    // Contribuição da luz ambiente
    glm::vec3 I_ambient = compMul(I_A, K_ambient);

    // Contribuição difusa
    float ln = glm::max(0.0f, glm::dot(l, n));
    glm::vec3 I_diff = compMul(I_F, K_diffuse) * ln;

    // Contribuição especular
    float vr = glm::max(0.0f, glm::dot(v, r));
    glm::vec3 I_spec = compMul(I_F, K_specular) * std::pow(vr, shininess);

    // Cor final
    glm::vec3 I_total = I_ambient + I_diff + I_spec;
    outColor = glm::clamp(I_total, glm::vec3(0.0f), glm::vec3(1.0f));

    return true;
}