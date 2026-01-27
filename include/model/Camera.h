#pragma once
#include <glm/glm.hpp>
#include "model/Ray.h"
#include "model/Point.h"

class Camera {
public:
    Point position;

    glm::vec3 forward;
    glm::vec3 right;
    glm::vec3 up;

    float fovY;     
    float aspect;    
    float nearDist;

    float yaw;  
    float pitch;

	bool isOrtographic = false;

    Camera(
        const Point& pos,
        const glm::vec3& lookAt,
        const glm::vec3& upVec,
        float fovY,
        float aspect,
        float nearDist,
		bool isOrtographic
    );

    Ray generateRay(float px, float py) const;

    void moveForward(float delta);

    void moveRight(float delta);

    void rotate(float deltaYaw, float deltaPitch);
};
