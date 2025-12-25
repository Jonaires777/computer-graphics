#pragma once
#include <glm/glm.hpp>
#include "model/Point.h"
#include "model/Ray.h"

struct Light {
    glm::vec3 intensity;

    Light(const glm::vec3& intensity)
        : intensity(intensity) {
    }

    virtual ~Light() = default;

    virtual bool illuminate(
        const glm::vec3& Pi,
        glm::vec3& directionToLight,
        float& distanceToLight,
        float& attenuation
    ) const = 0;
};
