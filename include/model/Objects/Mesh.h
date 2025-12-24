#pragma once
#include "model/Objects/Object.h"
#include "model/Objects/Triangle.h"
#include <vector>
#include <limits>

struct HitRecord {
    float t = std::numeric_limits<float>::infinity();
    const Triangle* hitTriangle = nullptr;
};

class Mesh : public Object {
public:
    std::vector<Triangle> triangles;

    Mesh(const std::vector<Triangle>& tris);

    // Compatibilidade com Object (Assinatura Original)
    // NOTA: Esta função sozinha não é ideal para threads se salvar estado no objeto.
    virtual bool intersect(const Ray& ray, float& t_out) const override;
    virtual glm::vec3 getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const override;

    // Versões Otimizadas para Threads (Passando HitRecord explicitamente)
    bool intersect(const Ray& ray, HitRecord& hit) const;
    glm::vec3 getNormalFromHit(const HitRecord& hit, const glm::vec3& Pi) const;
};