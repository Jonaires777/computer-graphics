#pragma once
#include "model/Point.h"
#include "model/Light.h"
#include <glm/glm.hpp>

struct SpotLight : public Light {
    Point position;
    glm::vec3 direction; 
    float cutoffCos;

    SpotLight(
        const glm::vec3& intensity,
        const Point& pos,
        const glm::vec3& dir,
        float cutoffDeg
    )
        : Light(intensity),
        position(pos),
        direction(glm::normalize(dir))
    {
        float radians = glm::radians(cutoffDeg);
        cutoffCos = cos(radians);
    }

    bool illuminate(
        const glm::vec3& Pi,
        glm::vec3& dirToLight,
        float& dist,
        float& attenuation
    ) const override
    {
        glm::vec3 L = glm::vec3(position.position) - Pi;
        dist = glm::length(L);

        dirToLight = glm::normalize(L);

        glm::vec3 lightToPoint = glm::normalize(Pi - glm::vec3(position.position));

        float cosAngle = glm::dot(glm::normalize(direction), lightToPoint);

        if (cosAngle < cutoffCos)
            return false;

        attenuation = 1.0f / (dist * dist);

        return true;
    }
};
