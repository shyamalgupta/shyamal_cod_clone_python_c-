#pragma once
#include "particles.h"
#include "raylib.h"
#include "raymath.h"
#include <vector>


struct Grenade {
  Vector3 position;
  Vector3 velocity;
  float fuseTimer;
  bool active;
  float bounceElasticity;
  float radius; // visual radius
  float blastRadius;
  float damage;
};

struct GrenadeSystem {
  std::vector<Grenade> grenades;
  int playerGrenades;
  int maxGrenades;
  float throwCooldown;
};

void GrenadeSystemInit(GrenadeSystem &gs);
void GrenadeSystemUpdate(GrenadeSystem &gs, float dt,
                         const std::vector<BoundingBox> &colliders);
void GrenadeSystemDraw(const GrenadeSystem &gs);
void GrenadeThrow(GrenadeSystem &gs, Vector3 origin, Vector3 direction,
                  float throwForce);

// Returns explosion positions for this frame (to apply damage externally)
struct GrenadeExplosion {
  Vector3 position;
  float blastRadius;
  float damage;
};
std::vector<GrenadeExplosion> GrenadeGetExplosions(GrenadeSystem &gs);
