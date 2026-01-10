#include "model/Objects/Mesh.h"

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <stb_image/stb_image.h>
#include <tiny_obj_loader/tiny_obj_loader.h>

#include <algorithm>
#include <limits>

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
    if (this->intersectWithHitRecord(ray, tempHit)) {
        t_out = tempHit.t;
        return true;
    }
    return false;
}

// Implementação da classe base (PROBLEMA: Aqui não sabemos qual triângulo foi atingido)
// Para funcionar via Object*, você teria que salvar o hit no Mesh (o que quebra as threads).
// Por isso, usaremos o getNormalFromHit no renderRows.
glm::vec3 Mesh::getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const {
    return glm::vec3(0.0f);
}

bool Mesh::intersectWithHitRecord(const Ray& ray, HitRecord& hit) const {
    float tNear, tFar;
    if (!boundingBox.intersect(ray, tNear, tFar)) {
        return false;
    }

    bool hasHit = false;
    for (const auto& tri : triangles) {
        float t_tri, u_tri, v_tri;
        if (tri.intersectBaricentric(ray, t_tri, u_tri, v_tri)) {
            if (t_tri < hit.t && t_tri > 0.001f) {
                hit.t = t_tri;
                hit.hitTriangle = &tri;
                hit.u = u_tri;
                hit.v = v_tri;
                hasHit = true;
            }
        }
    }
    return hasHit;
}

glm::vec3 Mesh::getNormalFromHit(const HitRecord& hit, const glm::vec3& Pi) const {
    if (!hit.hitTriangle) return glm::vec3(0.0f);

    glm::vec3 nLocal = hit.hitTriangle->getNormal(Pi, glm::vec3(0.0f));

    glm::vec3 nWorld = glm::normalize(this->normalMatrix * nLocal);

    return nWorld;
}

AABB Mesh::getAABB() const {
    return boundingBox;
}

void Mesh::updateAABB() {
    if (triangles.empty()) return;

    boundingBox.min = glm::vec3(std::numeric_limits<float>::max());
    boundingBox.max = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& tri : triangles) {
        boundingBox.min = glm::min(boundingBox.min, glm::vec3(tri.v0.position));
        boundingBox.max = glm::max(boundingBox.max, glm::vec3(tri.v0.position));

        boundingBox.min = glm::min(boundingBox.min, glm::vec3(tri.v1.position));
        boundingBox.max = glm::max(boundingBox.max, glm::vec3(tri.v1.position));

        boundingBox.min = glm::min(boundingBox.min, glm::vec3(tri.v2.position));
        boundingBox.max = glm::max(boundingBox.max, glm::vec3(tri.v2.position));
    }
}

glm::vec3 Mesh::getDiffuseColor(const HitRecord& hit) const {
    if (hasTexture && textureData) {
        const Triangle* tri = hit.hitTriangle;
        float w = 1.0f - hit.u - hit.v;
        glm::vec2 uv = w * tri->uv0 + hit.u * tri->uv1 + hit.v * tri->uv2;

        float u_wrapped = uv.x - std::floor(uv.x);
        float v_wrapped = uv.y - std::floor(uv.y);

        int x = static_cast<int>(u_wrapped * (texWidth - 1));
        int y = static_cast<int>(v_wrapped * (texHeight - 1));

        int idx = (y * texWidth + x) * 3;

        return glm::vec3(
            textureData[idx] / 255.0f,
            textureData[idx + 1] / 255.0f,
            textureData[idx + 2] / 255.0f
        );
    }
    
    return this->K_diffuse;
}

void Mesh::loadTexture(const std::string& path) {
    if (textureData) {
        stbi_image_free(textureData);
    }

    stbi_set_flip_vertically_on_load(true);
    textureData = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, 3);

    if (textureData) {
        hasTexture = true;
    }
    else {
        std::cerr << "Erro ao carregar textura: " << path << std::endl;
        hasTexture = false;
    }
}

void Mesh::loadFromObj(const std::string& path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        std::cerr << "Erro ao carregar OBJ: " << err << std::endl;
        return;
    }

    this->triangles.clear();

    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
            tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
            tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

            Point p0(attrib.vertices[3 * idx0.vertex_index + 0], attrib.vertices[3 * idx0.vertex_index + 1], attrib.vertices[3 * idx0.vertex_index + 2]);
            Point p1(attrib.vertices[3 * idx1.vertex_index + 0], attrib.vertices[3 * idx1.vertex_index + 1], attrib.vertices[3 * idx1.vertex_index + 2]);
            Point p2(attrib.vertices[3 * idx2.vertex_index + 0], attrib.vertices[3 * idx2.vertex_index + 1], attrib.vertices[3 * idx2.vertex_index + 2]);

            glm::vec2 uv0(0), uv1(0), uv2(0);
            if (idx0.texcoord_index >= 0) {
                uv0 = { attrib.texcoords[2 * idx0.texcoord_index + 0], attrib.texcoords[2 * idx0.texcoord_index + 1] };
                uv1 = { attrib.texcoords[2 * idx1.texcoord_index + 0], attrib.texcoords[2 * idx1.texcoord_index + 1] };
                uv2 = { attrib.texcoords[2 * idx2.texcoord_index + 0], attrib.texcoords[2 * idx2.texcoord_index + 1] };
            }

            this->triangles.push_back(Triangle(p0, p1, p2, uv0, uv1, uv2,
                glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(0.5f), 32.0f));
        }
    }
    this->updateAABB();
}