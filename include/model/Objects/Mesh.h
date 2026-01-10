#pragma once
#include "model/Objects/Object.h"
#include "model/Objects/Triangle.h"
#include <vector>
#include <limits>
#include "model/AABB.h"
#include <iostream>

struct HitRecord {
    float t = std::numeric_limits<float>::infinity();
    const Triangle* hitTriangle = nullptr;
    float u, v;
};

class Mesh : public Object {
public:
    std::vector<Triangle> triangles;
    AABB boundingBox;

    unsigned char* textureData = nullptr;
    int texWidth = 0, texHeight = 0, texChannels = 0;
    bool hasTexture = false;

    Mesh(const std::vector<Triangle>& tris);

    // Compatibilidade com Object (Assinatura Original)
    // NOTA: Esta função sozinha não é ideal para threads se salvar estado no objeto.
    virtual bool intersect(const Ray& ray, float& t_out) const override;
    virtual glm::vec3 getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const override;

    // Versões Otimizadas para Threads (Passando HitRecord explicitamente)
    bool intersectWithHitRecord(const Ray& ray, HitRecord& hit) const;
    glm::vec3 getNormalFromHit(const HitRecord& hit, const glm::vec3& Pi) const;

    void loadTexture(const std::string& path);

    void loadFromObj(const std::string& path);

    glm::vec3 getDiffuseColor(const HitRecord& hit) const;

    void updateAABB();

    virtual AABB getAABB() const override;
};