#pragma once
#include "model/Point.h"
#include "model/Light.h"
#include <glm/glm.hpp>

struct PointLight : public Light {
    Point position;

    PointLight(const glm::vec3& intensity, const Point& pos)
        : Light(intensity), position(pos) {
    }

    bool illuminate(
        const glm::vec3& Pi,
        glm::vec3& dir,
        float& dist,
        float& attenuation
    ) const override
    {
        glm::vec3 L = glm::vec3(position.position) - Pi;
        dist = glm::length(L);
        dir = glm::normalize(L);

        attenuation = 1.0f; 
        return true;        
    }
};