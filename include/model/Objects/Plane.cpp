#include "model/Objects/Plane.h"
#include "model/Point.h"
#include "operations/Operations.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

bool Plane::intersect(const Ray& ray, float& t_out) const {
    glm::vec3 O = glm::vec3(ray.origin.position);
    glm::vec3 D = glm::vec3(ray.direction);
    D = glm::normalize(D);

    glm::vec3 P0 = glm::vec3(point_pi.position);
    glm::vec3 n = glm::normalize(glm::vec3(normal_n));

    float denom = glm::dot(D, n);
    if (fabs(denom) < 1e-6f) {
        return false;
    }

    t_out = glm::dot(P0 - O, n) / denom;
    return t_out > 1e-4f;
}

bool Plane::shade(const glm::vec3& Pi, const Ray& ray, const LightSource& light, const glm::vec3& I_A, const Sphere& sphere, glm::vec3& outColor) const {
    using namespace Operations;

    glm::vec3 D = glm::vec3(ray.direction);
    D = glm::normalize(D);
    glm::vec3 n = glm::normalize(glm::vec3(normal_n));

    glm::vec3 Pf = glm::vec3(light.position.position);
    glm::vec3 l = glm::normalize(Pf - Pi);
    glm::vec3 v = glm::normalize(-D);

    glm::vec3 I_ambient = compMul(I_A, K_ambient);
    glm::vec3 I_diff_spec = glm::vec3(0.0f);

    float t_occlusion_sphere;
    
    glm::vec3 shadow_origin_pos = Pi + 1e-4f * l;
    Ray shadow_ray = { Point(shadow_origin_pos.x, shadow_origin_pos.y, shadow_origin_pos.z, 1.0f), glm::vec4(l, 0.0f) };

    if (!sphere.intersect(shadow_ray, t_occlusion_sphere) || t_occlusion_sphere > glm::distance(Pi, Pf)) {
        glm::vec3 r = glm::normalize(2.0f * glm::dot(n, l) * n - l);
        glm::vec3 I_F = light.intensity;
        glm::vec3 I_diff = compMul(I_F, K_diffuse) * glm::max(0.0f, glm::dot(l, n));
        glm::vec3 I_spec = compMul(I_F, K_specular) * std::pow(glm::max(0.0f, glm::dot(v, r)), shininess);
        I_diff_spec = I_diff + I_spec;
    }

    outColor = glm::clamp(I_ambient + I_diff_spec, glm::vec3(0.0f), glm::vec3(1.0f));
    return true;
}
