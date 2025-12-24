#include <glm/glm.hpp>
#include "model/Camera.h"

Camera::Camera(
    const Point& pos,
    const glm::vec3& lookAt,
    const glm::vec3& upVec,
    float fovY,
    float aspect,
    float nearDist
)
    : position(pos),
    fovY(fovY),
    aspect(aspect),
    nearDist(nearDist)
{
    forward = glm::normalize(lookAt - glm::vec3(pos.position));
    right = glm::normalize(glm::cross(forward, upVec));
    up = glm::normalize(glm::cross(right, forward));
}

Ray Camera::generateRay(float px, float py) const
{
    float h = 2.0f * nearDist * tan(fovY / 2.0f);
    float w = h * aspect;

    glm::vec3 imagePoint =
        glm::vec3(position.position) +
        forward * nearDist +
        right * (px * w * 0.5f) +
        up * (py * h * 0.5f);

    glm::vec3 dir = glm::normalize(imagePoint - glm::vec3(position.position));

    return Ray(position, glm::vec4(dir, 0.0f));
}

void Camera::moveForward(float delta) {
    position.position += glm::vec4(forward * delta, 0.0f);
}

void Camera::moveRight(float delta) {
    position.position += glm::vec4(right * delta, 0.0f);
}
