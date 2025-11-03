#pragma once
#include "model/Objects/Object.h"
#include "model/Objects/Triangle.h"
#include <vector>

struct Mesh : public Object {
    std::vector<Triangle> triangles;

    Mesh() = default;

    Mesh(const std::vector<Triangle>& tris);

    bool intersect(const Ray& ray, float& t_out) const override;
    glm::vec3 getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const override;
};
