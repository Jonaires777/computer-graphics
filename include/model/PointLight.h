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

        float constant = 1.0f;
        float linear = 0.7f;   
        float quadratic = 1.8f; 

        attenuation = 1.0f / (constant + linear * dist + quadratic * (dist * dist));

        return true;        
    }
};