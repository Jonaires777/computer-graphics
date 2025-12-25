#pragma once
#include "model/Point.h"
#include "model/Light.h"
#include <glm/glm.hpp>

struct DirectionalLight : public Light {
    glm::vec3 direction;

    DirectionalLight(const glm::vec3& intensity, const glm::vec3& dir)
        : Light(intensity), direction(glm::normalize(dir)) {
    }

    bool illuminate(
        const glm::vec3& Pi,
        glm::vec3& dirToLight,
        float& dist,
        float& attenuation
    ) const override
    {
        dirToLight = -direction;
        dist = INFINITY;
        attenuation = 1.0f;
        return true;
    }
};
