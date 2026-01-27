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
	isOrtographic(isOrtographic),
    currentProj(isOrtographic ? PROJ_ORTHOGRAPHIC : PROJ_PERSPECTIVE)
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
    // 1. Definição do tamanho da janela de visão
    float h, w;

    // Se for Perspectiva, a altura depende do ângulo (FOV) e da distância
    if (currentProj == PROJ_PERSPECTIVE) {
        h = 2.0f * nearDist * tan(fovY / 2.0f);
    }
    // Se for Ortográfica ou Oblíqua, a altura é fixa (funciona como um Zoom)
    else {
        h = 7.0f; // Tamanho da área visível em metros (mude isso para dar zoom)
    }

    w = h * aspect;

    // ---------------------------------------------------------
    // CASO 1: PERSPECTIVA (Cônica - Realista)
    // ---------------------------------------------------------
    if (currentProj == PROJ_PERSPECTIVE) {
        // Ponto alvo na tela virtual
        glm::vec3 imagePoint =
            glm::vec3(position.position) +
            forward * nearDist +
            right * (px * w * 0.5f) +
            up * (py * h * 0.5f);

        // A origem é fixa (o olho da câmera)
        // A direção varia para cada pixel
        glm::vec3 dir = glm::normalize(imagePoint - glm::vec3(position.position));

        return Ray(position, glm::vec4(dir, 0.0f));
    }

    // ---------------------------------------------------------
    // CASO 2 E 3: PROJEÇÕES PARALELAS (Ortográfica e Oblíqua)
    // ---------------------------------------------------------
    else {
        // A origem do raio "anda" pela grade da tela
        glm::vec3 rayOrigin =
            glm::vec3(position.position) +
            right * (px * w * 0.5f) +
            up * (py * h * 0.5f);

        glm::vec3 dir;

        if (currentProj == PROJ_ORTHOGRAPHIC) {
            // Ortográfica: Raios 100% retos para frente
            dir = forward;
        }
        else { // PROJ_OBLIQUE
            // Oblíqua: Raios paralelos, mas inclinados (Shear)
            // 0.5 em X e Y dá uma inclinação de ~45 graus
            float shearX = 0.5f;
            float shearY = 0.5f;

            // Adicionamos um desvio lateral ao vetor forward
            dir = glm::normalize(forward + (right * shearX) + (up * shearY));
        }

        return Ray(Point(rayOrigin.x, rayOrigin.y, rayOrigin.z, 1.0f), glm::vec4(dir, 0.0f));
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
