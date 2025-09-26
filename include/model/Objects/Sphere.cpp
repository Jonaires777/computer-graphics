#include "model/Objects/Sphere.h"
#include "operations/Operations.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

bool Sphere::shade(const Ray& ray, const LightSource& light, glm::vec3& outColor) const {
    using namespace Operations;
    
    glm::vec3 O = glm::vec3(ray.origin.position);
    glm::vec3 D = glm::vec3(ray.direction);
    if (glm::length(D) == 0.0f) return false;
    D = glm::normalize(D);

	glm::vec3 C = glm::vec3(center.position);

    glm::vec3 oc = O - C;
    float a = glm::dot(D, D);
    float b = 2.0f * glm::dot(oc, D);
    float c = glm::dot(oc, oc) - radius * radius;

    float disc = b * b - 4 * a * c;
    if (disc < 0.0f) return false;

    float sqrtDisc = std::sqrt(disc);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);

    float t = (t1 > 1e-4f) ? t1 : ((t2 > 1e-4f) ? t2 : -1.0f);
    if (t < 0.0f) return false;

	// intersection point
    glm::vec3 Pi = O + t * D;

	// normal at intersection
    glm::vec3 n = glm::normalize(Pi - C);

    // l = normalize(Pf - Pi)
    glm::vec3 Pf = glm::vec3(light.position.position);
    glm::vec3 l = glm::normalize(Pf - Pi);

    // v = -D (eye direction)
    glm::vec3 v = glm::normalize(-D);

	// r = normalize(2 * glm::dot(n, l) * n - l)
	glm::vec3 r = glm::normalize(2 * glm::dot(n, l) * n - l);

    // lightsource intensity
    glm::vec3 I_F = light.intensity;

    // ambient (I_F @ Ka)
    glm::vec3 I_ambient = compMul(I_F, K_ambient);

    // diffuse: (I_F @ Kd) * max(0, l.n)
    float ln = glm::max(0.0f, glm::dot(l, n));
    glm::vec3 I_diff = compMul(I_F, K_diffuse) * ln;

    // specular: (I_F @ Ks) * (max(0, v.r)^m)
    float vr = glm::max(0.0f, glm::dot(v, r));
    glm::vec3 I_spec = compMul(I_F, K_specular) * std::pow(vr, shininess);

    glm::vec3 I_total = I_ambient + I_diff + I_spec;
    
	outColor = glm::clamp(I_total, glm::vec3(0.0f), glm::vec3(1.0f));
	return true;
}