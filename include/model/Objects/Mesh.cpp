#include "model/Objects/Mesh.h"
#include <limits>

Mesh::Mesh(const std::vector<Triangle>& tris)
    : triangles(tris)
{
}

bool Mesh::intersect(const Ray& ray, float& t_out) const {
   bool hit = false;
   float t_min = std::numeric_limits<float>::max();

   for (const auto& tri : triangles) {
       float t;
       if (tri.intersect(ray, t) && t < t_min) {
           t_min = t;
           hit = true;\

           const_cast<glm::vec3&>(K_ambient) = tri.K_ambient;
           const_cast<glm::vec3&>(K_diffuse) = tri.K_diffuse;
           const_cast<glm::vec3&>(K_specular) = tri.K_specular;
           const_cast<float&>(shininess) = tri.shininess;
       }
   }

   if (hit) {
       t_out = t_min;
       return true;
   }

   return false;
}

glm::vec3 Mesh::getNormal(const glm::vec3& Pi, const glm::vec3& rayDir) const {
    const Triangle* nearest = nullptr;
    float t_min = std::numeric_limits<float>::max();

    for (const auto& tri : triangles) {
        float t;
        if (tri.intersect(Ray(Point(Pi.x, Pi.y, Pi.z, 1.0f), glm::vec4(rayDir, 0.0f)), t) && t < t_min) {
            t_min = t;
            nearest = &tri;
        }
    }

    if (nearest)
        return nearest->getNormal(Pi, rayDir);

    return glm::vec3(0.0f);
}
