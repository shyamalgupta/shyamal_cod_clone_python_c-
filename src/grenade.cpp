#include "grenade.h"
#include <algorithm>
#include <math.h>
#include <stdlib.h>


void GrenadeSystemInit(GrenadeSystem &gs) {
  gs.grenades.clear();
  gs.playerGrenades = 4;
  gs.maxGrenades = 4;
  gs.throwCooldown = 0.0f;
}

void GrenadeSystemUpdate(GrenadeSystem &gs, float dt,
                         const std::vector<BoundingBox> &colliders) {
  if (gs.throwCooldown > 0)
    gs.throwCooldown -= dt;

  for (auto &g : gs.grenades) {
    if (!g.active)
      continue;

    // Save previous position
    Vector3 prevPos = g.position;

    // Apply gravity
    g.velocity.y -= 20.0f * dt;

    // Move
    g.position = Vector3Add(g.position, Vector3Scale(g.velocity, dt));

    // Ground bounce
    if (g.position.y <= g.radius) {
      g.position.y = g.radius;
      g.velocity.y = -g.velocity.y * g.bounceElasticity;
      g.velocity.x *= 0.7f; // friction on bounce
      g.velocity.z *= 0.7f;

      // Stop bouncing if very slow
      if (fabsf(g.velocity.y) < 0.5f) {
        g.velocity.y = 0;
        g.position.y = g.radius;
      }
    }

    // Wall collision (simplified)
    for (const auto &box : colliders) {
      // Check if grenade is inside a collider
      if (g.position.x > box.min.x && g.position.x < box.max.x &&
          g.position.y > box.min.y && g.position.y < box.max.y &&
          g.position.z > box.min.z && g.position.z < box.max.z) {
        // Push back to previous position and bounce
        g.position = prevPos;

        // Determine which face was hit and bounce
        float dx1 = fabsf(g.position.x - box.min.x);
        float dx2 = fabsf(g.position.x - box.max.x);
        float dz1 = fabsf(g.position.z - box.min.z);
        float dz2 = fabsf(g.position.z - box.max.z);

        float minD = fminf(fminf(dx1, dx2), fminf(dz1, dz2));
        if (minD == dx1 || minD == dx2) {
          g.velocity.x = -g.velocity.x * g.bounceElasticity;
        } else {
          g.velocity.z = -g.velocity.z * g.bounceElasticity;
        }
        g.velocity = Vector3Scale(g.velocity, 0.6f); // energy loss
        break;
      }
    }

    // Fuse countdown
    g.fuseTimer -= dt;
  }
}

void GrenadeSystemDraw(const GrenadeSystem &gs) {
  for (const auto &g : gs.grenades) {
    if (!g.active)
      continue;

    // Grenade body (dark olive sphere)
    DrawSphere(g.position, g.radius, (Color){60, 80, 50, 255});
    DrawSphereWires(g.position, g.radius + 0.005f, 6, 6,
                    (Color){40, 50, 30, 200});

    // Fuse spark (flashing)
    if (fmodf(g.fuseTimer, 0.3f) < 0.15f) {
      Vector3 sparkPos = {g.position.x, g.position.y + g.radius, g.position.z};
      DrawSphere(sparkPos, 0.03f, (Color){255, 200, 50, 255});
    }

    // Danger indicator when about to explode
    if (g.fuseTimer < 1.0f) {
      float pulse = (sinf(g.fuseTimer * 20.0f) + 1.0f) * 0.5f;
      unsigned char alpha = (unsigned char)(60 * pulse);
      DrawSphere(g.position, g.blastRadius * 0.1f, (Color){255, 50, 30, alpha});
    }
  }
}

void GrenadeThrow(GrenadeSystem &gs, Vector3 origin, Vector3 direction,
                  float throwForce) {
  if (gs.playerGrenades <= 0)
    return;
  if (gs.throwCooldown > 0)
    return;

  Grenade g;
  g.position = Vector3Add(origin, Vector3Scale(direction, 1.0f));
  g.position.y -= 0.2f; // slightly below eye level

  // Throw velocity = forward + slight upward arc
  g.velocity = Vector3Scale(direction, throwForce);
  g.velocity.y += throwForce * 0.4f; // arc upward

  g.fuseTimer = 3.0f;
  g.active = true;
  g.bounceElasticity = 0.35f;
  g.radius = 0.08f;
  g.blastRadius = 10.0f;
  g.damage = 120.0f;

  gs.grenades.push_back(g);
  gs.playerGrenades--;
  gs.throwCooldown = 0.5f;
}

std::vector<GrenadeExplosion> GrenadeGetExplosions(GrenadeSystem &gs) {
  std::vector<GrenadeExplosion> explosions;
  for (auto &g : gs.grenades) {
    if (!g.active)
      continue;
    if (g.fuseTimer <= 0) {
      GrenadeExplosion exp;
      exp.position = g.position;
      exp.blastRadius = g.blastRadius;
      exp.damage = g.damage;
      explosions.push_back(exp);
      g.active = false;
    }
  }
  // Clean up inactive grenades
  gs.grenades.erase(std::remove_if(gs.grenades.begin(), gs.grenades.end(),
                                   [](const Grenade &g) { return !g.active; }),
                    gs.grenades.end());
  return explosions;
}
