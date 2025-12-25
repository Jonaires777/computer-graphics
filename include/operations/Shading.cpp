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
    const std::vector<Light*>& lights,
    const glm::vec3& I_A,
    const Object& hitObject,
    const std::vector<std::unique_ptr<Object>>& sceneObjects
) {
    using namespace Operations;

    glm::vec3 D = glm::normalize(glm::vec3(ray.direction));
    glm::vec3 v = glm::normalize(-D);

    glm::vec3 I_total = hitObject.K_ambient * I_A;

    for (const Light* light : lights)
    {
        glm::vec3 l;
        float dist;
        float attenuation;

        if (!light->illuminate(Pi, l, dist, attenuation))
            continue;

        glm::vec3 shadow_origin = Pi + 1e-4f * l;
        Ray shadow_ray = {
            Point(shadow_origin.x, shadow_origin.y, shadow_origin.z, 1.0f),
            glm::vec4(l, 0.0f)
        };

        bool inShadow = false;

        for (auto& obj : sceneObjects) {
            float t;
            if (obj->intersect(shadow_ray, t)) {
                if (t > 0.0f && t < dist) {
                    inShadow = true;
                    break;
                }
            }
        }

        if (inShadow)
            continue;

        glm::vec3 r = glm::normalize(2.0f * glm::dot(n, l) * n - l);

        glm::vec3 I_diff =
            compMul(light->intensity, hitObject.K_diffuse)
            * glm::max(0.0f, glm::dot(l, n));

        glm::vec3 I_spec =
            compMul(light->intensity, hitObject.K_specular)
            * std::pow(glm::max(0.0f, glm::dot(v, r)), hitObject.shininess);

        I_total += attenuation * (I_diff + I_spec);
    }

    return glm::clamp(I_total, glm::vec3(0), glm::vec3(1));
}
