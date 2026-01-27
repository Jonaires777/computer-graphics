#include <glm/glm.hpp>
#include "model/Camera.h"
#include <glm/gtc/constants.hpp>

Camera::Camera(
    const Point& pos,
    const glm::vec3& lookAt,
    const glm::vec3& upVec,
    float fovY,
    float aspect,
    float nearDist,
	bool isOrtographic
)
    : position(pos),
    fovY(fovY),
    aspect(aspect),
    nearDist(nearDist),
	isOrtographic(isOrtographic)
{
    forward = glm::normalize(lookAt - glm::vec3(pos.position));
    right = glm::normalize(glm::cross(forward, upVec));
    up = glm::normalize(glm::cross(right, forward));

    glm::vec3 dir = glm::normalize(lookAt - glm::vec3(pos.position));

    yaw = atan2(dir.z, dir.x);
    pitch = asin(dir.y);

    rotate(0, 0);
}

Ray Camera::generateRay(float px, float py) const
{
    if (isOrtographic) {
        float h = 7.0f;
		float w = h * aspect;
        glm::vec3 dir = forward;
		glm::vec3 rayOrigin =
			glm::vec3(position.position) +
			right * (px * w * 0.5f) +
			up * (py * h * 0.5f);
        return Ray(Point(rayOrigin.x, rayOrigin.y, rayOrigin.z, 1.0f), glm::vec4(dir, 0.0f));
    }
    else {
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
}

void Camera::moveForward(float delta) {
    position.position += glm::vec4(forward * delta, 0.0f);
}

void Camera::moveRight(float delta) {
    position.position += glm::vec4(right * delta, 0.0f);
}

void Camera::rotate(float deltaYaw, float deltaPitch)
{
    yaw += deltaYaw;
    pitch += deltaPitch;

    float limit = glm::radians(89.0f);
    pitch = glm::clamp(pitch, -limit, limit);

    forward = glm::normalize(glm::vec3(
        cos(pitch) * cos(yaw),
        sin(pitch),
        cos(pitch) * sin(yaw)
    ));

    right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    up = glm::normalize(glm::cross(right, forward));
}
