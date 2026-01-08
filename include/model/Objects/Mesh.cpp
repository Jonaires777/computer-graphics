#include "model/Objects/Mesh.h"

Mesh::Mesh(const std::vector<Triangle>& tris) : triangles(tris) {
    for (const auto& tri : triangles) {
        boundingBox.min = glm::min(boundingBox.min, glm::vec3(tri.v0.position));
        boundingBox.min = glm::min(boundingBox.min, glm::vec3(tri.v1.position));
        boundingBox.min = glm::min(boundingBox.min, glm::vec3(tri.v2.position));

        boundingBox.max = glm::max(boundingBox.max, glm::vec3(tri.v0.position));
        boundingBox.max = glm::max(boundingBox.max, glm::vec3(tri.v1.position));
        boundingBox.max = glm::max(boundingBox.max, glm::vec3(tri.v2.position));
    }
}

// Implementação da classe base (apenas para compatibilidade de interface)
bool Mesh::intersect(const Ray& ray, float& t_out) const {
    HitRecord tempHit;
    if (this->intersect(ray, tempHit)) {
        t_out = tempHit.t;
        return true;
    }
    return false;
}

// Implementação da classe base (PROBLEMA: Aqui não sabemos qual triângulo foi atingido)
// Para funcionar via Object*, você teria que salvar o hit no Mesh (o que quebra as threads).
// Por isso, usaremos o getNormalFromHit no renderRows.
glm::vec3 Mesh::getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const {
    return glm::vec3(0.0f); // Não deve ser chamada diretamente para Meshes em Multithread
}

// VERSÃO SEGURA PARA THREADS COM ACELERAÇÃO
bool Mesh::intersect(const Ray& ray, HitRecord& hit) const {
    // --- OTIMIZAÇÃO: TESTE DA CAIXA ---
    // Se o raio não atingir nem a caixa externa, ignore todos os triângulos
    float tNear, tFar;
    if (!boundingBox.intersect(ray, tNear, tFar)) {
        return false;
    }
    // ---------------------------------

    bool hasHit = false;
    for (const auto& tri : triangles) {
        float t_tri;
        if (tri.intersect(ray, t_tri)) {
            if (t_tri < hit.t && t_tri > 0.001f) {
                hit.t = t_tri;
                hit.hitTriangle = &tri;
                hasHit = true;
            }
        }
    }
    return hasHit;
}

glm::vec3 Mesh::getNormalFromHit(const HitRecord& hit, const glm::vec3& Pi) const {
    if (!hit.hitTriangle) return glm::vec3(0.0f);

    // Normal local do triângulo (certifique-se que Triangle::getNormal não inverte mais!)
    glm::vec3 nLocal = hit.hitTriangle->getNormal(Pi, glm::vec3(0.0f));

    // Usa a normalMatrix herdada de Object
    glm::vec3 nWorld = glm::normalize(this->normalMatrix * nLocal);

    // REMOVIDO: if (dot(nWorld, rayDir) > 0) nWorld = -nWorld;
    // A iluminação agora depende da orientação real do triângulo.
    return nWorld;
}

AABB Mesh::getAABB() const {
    return boundingBox;
}