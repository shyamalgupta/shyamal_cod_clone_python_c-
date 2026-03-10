#include "physics.h"
#include <math.h>

BoundingBox PhysicsCreateBBox(Vector3 pos, Vector3 size) {
    return {
        {pos.x - size.x / 2, pos.y, pos.z - size.z / 2},
        {pos.x + size.x / 2, pos.y + size.y, pos.z + size.z / 2}
    };
}

bool PhysicsCheckCollision(Vector3 pos, float radius, const std::vector<BoundingBox>& colliders) {
    BoundingBox playerBox = {
        {pos.x - radius, pos.y, pos.z - radius},
        {pos.x + radius, pos.y + 1.8f, pos.z + radius}
    };

    for (const auto& box : colliders) {
        if (CheckCollisionBoxes(playerBox, box)) {
            return true;
        }
    }
    return false;
}

Vector3 PhysicsSlideMove(Vector3 currentPos, Vector3 desiredPos, float radius,
                         const std::vector<BoundingBox>& colliders) {
    Vector3 result = desiredPos;

    // Try full movement first
    if (!PhysicsCheckCollision(desiredPos, radius, colliders)) {
        return desiredPos;
    }

    // Try X only
    Vector3 xOnly = {desiredPos.x, currentPos.y, currentPos.z};
    if (!PhysicsCheckCollision(xOnly, radius, colliders)) {
        result = xOnly;
        // Also try Z from the X position
        Vector3 xz = {desiredPos.x, currentPos.y, desiredPos.z};
        if (!PhysicsCheckCollision(xz, radius, colliders)) {
            return xz;
        }
        return result;
    }

    // Try Z only
    Vector3 zOnly = {currentPos.x, currentPos.y, desiredPos.z};
    if (!PhysicsCheckCollision(zOnly, radius, colliders)) {
        return zOnly;
    }

    // Can't move at all
    return currentPos;
}

bool PhysicsRayBoxIntersect(Ray ray, BoundingBox box, float* outDist) {
    RayCollision collision = GetRayCollisionBox(ray, box);
    if (collision.hit) {
        *outDist = collision.distance;
        return true;
    }
    return false;
}
