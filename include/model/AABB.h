#pragma once
#include <glm/glm.hpp>
#include "model/Ray.h"

struct AABB {
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);

    // Verifica se um raio atinge a caixa
    bool intersect(const Ray& ray, float& t_near, float& t_far) const {
        glm::vec3 invDir = 1.0f / glm::vec3(ray.direction);
        glm::vec3 t0 = (min - glm::vec3(ray.origin.position)) * invDir;
        glm::vec3 t1 = (max - glm::vec3(ray.origin.position)) * invDir;

        glm::vec3 tmin = glm::min(t0, t1);
        glm::vec3 tmax = glm::max(t0, t1);

        t_near = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
        t_far = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

        return t_near <= t_far && t_far > 0;
    }
};