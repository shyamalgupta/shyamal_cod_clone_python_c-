#pragma once
#include "raylib.h"
#include <vector>

BoundingBox PhysicsCreateBBox(Vector3 pos, Vector3 size);
bool PhysicsCheckCollision(Vector3 pos, float radius, const std::vector<BoundingBox>& colliders);
Vector3 PhysicsSlideMove(Vector3 currentPos, Vector3 desiredPos, float radius, const std::vector<BoundingBox>& colliders);
bool PhysicsRayBoxIntersect(Ray ray, BoundingBox box, float* outDist);
int PhysicsRaycastEnemies(Ray ray, float maxDist);
