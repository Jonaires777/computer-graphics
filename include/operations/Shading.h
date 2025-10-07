#pragma once
#include "model/Ray.h"
#include "model/LightSource.h"
#include "model/Objects/Object.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

glm::vec3 shade(
    const glm::vec3& Pi,
    const glm::vec3& n,
    const Ray& ray,
    const LightSource& light,
    const glm::vec3& I_A,
    const Object& hitObject,
    const std::vector<std::unique_ptr<Object>>& sceneObjects
);
