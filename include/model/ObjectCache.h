#pragma once
#include "model/AABB.h"
#include "model/Objects/Object.h"

struct ObjectCache {
    AABB box;
    Object* ptr;
    bool isPlane;
    bool isMesh;
};