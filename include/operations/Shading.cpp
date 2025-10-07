#include "operations/Shading.h"
#include "operations/Operations.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

glm::vec3 shade(
    const glm::vec3& Pi,
    const glm::vec3& n,
    const Ray& ray,
    const LightSource& light,
    const glm::vec3& I_A,
    const Object& hitObject,
    const std::vector<std::unique_ptr<Object>>& sceneObjects
) {
	using namespace Operations;
    
    glm::vec3 D = glm::normalize(glm::vec3(ray.direction));
    glm::vec3 Pf = glm::vec3(light.position.position);
    glm::vec3 l = glm::normalize(Pf - Pi);
    glm::vec3 v = glm::normalize(-D);

    glm::vec3 I_ambient = hitObject.K_ambient * I_A;
    glm::vec3 I_diff_spec(0.0f);

    glm::vec3 shadow_origin = Pi + 1e-4f * l;
    Ray shadow_ray = { Point(shadow_origin.x, shadow_origin.y, shadow_origin.z, 1.0f), glm::vec4(l, 0.0f) };
    bool inShadow = false;

    for (auto& obj : sceneObjects) {
        float t;
        if (obj->intersect(shadow_ray, t)) {
            float lightDist = glm::distance(Pi, Pf);
            if (t > 0.0f && t < lightDist) {
                inShadow = true;
                break;
            }
        }
    }

    if (!inShadow) {
        glm::vec3 r = glm::normalize(2.0f * glm::dot(n, l) * n - l);
        glm::vec3 I_F = light.intensity;
        glm::vec3 I_diff = compMul(I_F, hitObject.K_diffuse) * glm::max(0.0f, glm::dot(l, n));
        glm::vec3 I_spec = compMul(I_F, hitObject.K_specular) * std::pow(glm::max(0.0f, glm::dot(v, r)), hitObject.shininess);
        I_diff_spec = I_diff + I_spec;
    }

    return glm::clamp(I_ambient + I_diff_spec, glm::vec3(0.0f), glm::vec3(1.0f));
}
