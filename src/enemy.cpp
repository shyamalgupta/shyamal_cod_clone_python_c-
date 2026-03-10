#include "enemy.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static float RandF(float lo, float hi) {
  return lo + (float)rand() / RAND_MAX * (hi - lo);
}
static Vector3 RandPatrol(Vector3 center, float range) {
  return {center.x + RandF(-range, range), center.y,
          center.z + RandF(-range, range)};
}

static EnemyType GetRandomEnemyType(int wave) {
  int r = rand() % 100;
  if (wave <= 2) {
    return r < 70 ? EnemyType::SOLDIER : EnemyType::RUSHER;
  } else if (wave <= 5) {
    if (r < 40)
      return EnemyType::SOLDIER;
    if (r < 65)
      return EnemyType::RUSHER;
    if (r < 80)
      return EnemyType::HEAVY;
    if (r < 92)
      return EnemyType::SNIPER;
    return EnemyType::DOG;
  } else {
    if (r < 30)
      return EnemyType::SOLDIER;
    if (r < 50)
      return EnemyType::RUSHER;
    if (r < 65)
      return EnemyType::HEAVY;
    if (r < 80)
      return EnemyType::SNIPER;
    return EnemyType::DOG;
  }
}

static void EnemyInit(Enemy &e, Vector3 spawn, EnemyType type, float hMult,
                      float sMult, float aMult) {
  e.position = spawn;
  e.velocity = {0, 0, 0};
  e.yaw = RandF(0, 6.28f);
  e.type = type;
  e.state = EnemyState::PATROL;
  e.active = true;
  e.hitFlashTimer = 0;
  e.deathTimer = 0;
  e.deathAnimTimer = 0;
  e.respawnTime = 8.0f;
  e.spawnPoint = spawn;
  e.strafePhase = RandF(0, 6.28f);
  e.strafeSpeed = RandF(1.5f, 3.0f);
  e.legAnimTimer = 0;
  e.patrolTarget = RandPatrol(spawn, 8.0f);
  e.patrolWaitTimer = 0;
  e.flankTarget = spawn;
  e.flankTimer = 0;
  e.coverPoint = spawn;
  e.inCover = false;
  e.coverTimer = 0;
  e.biteTimer = 0;
  e.biteCooldown = 1.2f;

  switch (type) {
  case EnemyType::SOLDIER:
    e.health = e.maxHealth = 100 * hMult;
    e.speed = 3.5f * sMult;
    e.radius = 0.3f;
    e.height = 1.8f;
    e.detectionRange = 28;
    e.attackRange = 20;
    e.accuracy = 0.55f * aMult;
    e.reactionTime = 0.6f;
    e.fireRate = 1.5f;
    e.damage = 8;
    e.color = {180, 60, 60, 255};
    e.accentColor = {140, 80, 50, 255};
    break;
  case EnemyType::RUSHER:
    e.health = e.maxHealth = 65 * hMult;
    e.speed = 6.5f * sMult;
    e.radius = 0.25f;
    e.height = 1.7f;
    e.detectionRange = 35;
    e.attackRange = 22;
    e.accuracy = 0.45f * aMult;
    e.reactionTime = 0.3f;
    e.fireRate = 2.0f;
    e.damage = 10;
    e.color = {50, 160, 60, 255};
    e.accentColor = {30, 110, 40, 255};
    break;
  case EnemyType::HEAVY:
    e.health = e.maxHealth = 220 * hMult;
    e.speed = 2.0f * sMult;
    e.radius = 0.4f;
    e.height = 2.0f;
    e.detectionRange = 22;
    e.attackRange = 15;
    e.accuracy = 0.6f * aMult;
    e.reactionTime = 0.9f;
    e.fireRate = 1.0f;
    e.damage = 18;
    e.color = {90, 90, 100, 255};
    e.accentColor = {60, 60, 70, 255};
    break;
  case EnemyType::SNIPER:
    e.health = e.maxHealth = 70 * hMult;
    e.speed = 2.5f * sMult;
    e.radius = 0.28f;
    e.height = 1.8f;
    e.detectionRange = 60;
    e.attackRange = 55;
    e.accuracy = 0.88f * aMult;
    e.reactionTime = 1.2f;
    e.fireRate = 0.5f;
    e.damage = 45;
    e.color = {50, 70, 100, 255};
    e.accentColor = {35, 55, 80, 255};
    break;
  case EnemyType::DOG:
    e.health = e.maxHealth = 45 * hMult;
    e.speed = 9.0f * sMult;
    e.radius = 0.25f;
    e.height = 0.7f;
    e.detectionRange = 30;
    e.attackRange = 1.2f;
    e.accuracy = 1.0f;
    e.reactionTime = 0.15f;
    e.fireRate = 0;
    e.damage = 20;
    e.color = {130, 90, 50, 255};
    e.accentColor = {100, 70, 40, 255};
    break;
  }
  e.reactionTimer = e.reactionTime;
  e.fireCooldown = 0;
}

// ---- PATROL ----
static void UpdatePatrol(Enemy &e, Vector3 playerPos, float dt) {
  float dx = e.patrolTarget.x - e.position.x;
  float dz = e.patrolTarget.z - e.position.z;
  float dist = sqrtf(dx * dx + dz * dz);
  float playerDist = sqrtf(powf(playerPos.x - e.position.x, 2) +
                           powf(playerPos.z - e.position.z, 2));

  if (playerDist < e.detectionRange) {
    e.state = EnemyState::ALERT;
    return;
  }
  if (dist < 0.5f) {
    e.patrolWaitTimer -= dt;
    if (e.patrolWaitTimer <= 0) {
      e.patrolTarget = RandPatrol(e.spawnPoint, 12.0f);
      e.patrolWaitTimer = RandF(1.5f, 3.5f);
    }
  } else {
    e.yaw = atan2f(dx, dz);
    e.position.x += sinf(e.yaw) * e.speed * 0.5f * dt;
    e.position.z += cosf(e.yaw) * e.speed * 0.5f * dt;
  }
  e.strafePhase += dt * 0.8f;
}

// ---- ALERT ----
static void UpdateAlert(Enemy &e, Vector3 playerPos, float dt) {
  float dx = playerPos.x - e.position.x;
  float dz = playerPos.z - e.position.z;
  float dist = sqrtf(dx * dx + dz * dz);

  if (dist > e.detectionRange * 1.5f) {
    e.state = EnemyState::PATROL;
    return;
  }

  e.yaw = atan2f(dx, dz);
  e.reactionTimer -= dt;
  if (e.reactionTimer <= 0 && dist < e.attackRange) {
    // Decide: attack or flank
    if (rand() % 100 < 30 && e.type != EnemyType::DOG) {
      e.state = EnemyState::FLANK;
      float flankSide = (rand() % 2 == 0) ? 1.0f : -1.0f;
      e.flankTarget = {playerPos.x + cosf(e.yaw) * 8.0f * flankSide, 0,
                       playerPos.z - sinf(e.yaw) * 8.0f * flankSide};
      e.flankTimer = 3.0f;
    } else {
      e.state = EnemyState::ATTACK;
    }
    e.reactionTimer = e.reactionTime;
  }

  // Move toward player in alert
  float moveSpeed = e.speed * 0.7f;
  if (dist > e.attackRange * 0.6f) {
    e.position.x += sinf(e.yaw) * moveSpeed * dt;
    e.position.z += cosf(e.yaw) * moveSpeed * dt;
  }
  e.strafePhase += dt * 2.0f;
}

// ---- FLANK ----
static void UpdateFlank(Enemy &e, Vector3 playerPos, float dt) {
  e.flankTimer -= dt;
  if (e.flankTimer <= 0) {
    e.state = EnemyState::ATTACK;
    return;
  }

  float dx = e.flankTarget.x - e.position.x;
  float dz = e.flankTarget.z - e.position.z;
  float dist = sqrtf(dx * dx + dz * dz);

  if (dist < 1.0f) {
    e.state = EnemyState::ATTACK;
    return;
  }

  e.yaw = atan2f(dx, dz);
  e.position.x += sinf(e.yaw) * e.speed * dt;
  e.position.z += cosf(e.yaw) * e.speed * dt;
  e.strafePhase += dt * 5.0f;

  // Keep facing player
  float pdx = playerPos.x - e.position.x;
  float pdz = playerPos.z - e.position.z;
  e.yaw = atan2f(pdx, pdz);
}

// ---- ATTACK ----
static void UpdateAttack(Enemy &e, Vector3 playerPos, float dt) {
  float dx = playerPos.x - e.position.x;
  float dz = playerPos.z - e.position.z;
  float dist = sqrtf(dx * dx + dz * dz);

  e.yaw = atan2f(dx, dz);
  if (dist > e.detectionRange * 2.0f) {
    e.state = EnemyState::PATROL;
    return;
  }

  e.strafePhase += dt * e.strafeSpeed;

  if (e.type == EnemyType::RUSHER || e.type == EnemyType::DOG) {
    // Rush directly
    if (dist > 1.0f) {
      e.position.x += sinf(e.yaw) * e.speed * dt;
      e.position.z += cosf(e.yaw) * e.speed * dt;
    }
  } else if (e.type == EnemyType::SNIPER) {
    // Stay at long range, strafe
    float idealDist = 30.0f;
    if (dist < idealDist - 5.0f) {
      // Back away
      e.position.x -= sinf(e.yaw) * e.speed * 0.4f * dt;
      e.position.z -= cosf(e.yaw) * e.speed * 0.4f * dt;
    }
    // Side strafe
    float strafeDir = sinf(e.strafePhase) * e.speed * 0.6f * dt;
    e.position.x += cosf(e.yaw) * strafeDir;
    e.position.z -= sinf(e.yaw) * strafeDir;

    // Enter cover when hurt
    if (e.health < e.maxHealth * 0.5f && !e.inCover) {
      e.inCover = true;
      e.coverTimer = 3.0f;
      e.coverPoint = {e.position.x - sinf(e.yaw) * 5.0f, 0,
                      e.position.z - cosf(e.yaw) * 5.0f};
    }
  } else if (e.type == EnemyType::HEAVY) {
    // Advance slowly
    if (dist > 6.0f) {
      e.position.x += sinf(e.yaw) * e.speed * dt;
      e.position.z += cosf(e.yaw) * e.speed * dt;
    }
  } else {
    // Soldier: strafe while firing
    float strafeOffset = sinf(e.strafePhase) * e.speed * 0.5f * dt;
    e.position.x += cosf(e.yaw) * strafeOffset;
    e.position.z -= sinf(e.yaw) * strafeOffset;

    float preferDist = 12.0f;
    if (dist > preferDist + 3.0f) {
      e.position.x += sinf(e.yaw) * e.speed * 0.5f * dt;
      e.position.z += cosf(e.yaw) * e.speed * 0.5f * dt;
    } else if (dist < preferDist - 3.0f) {
      e.position.x -= sinf(e.yaw) * e.speed * 0.3f * dt;
      e.position.z -= cosf(e.yaw) * e.speed * 0.3f * dt;
    }
  }

  // Cover behavior
  if (e.inCover) {
    e.coverTimer -= dt;
    if (e.coverTimer <= 0)
      e.inCover = false;
    // Move toward cover point
    float cdx = e.coverPoint.x - e.position.x;
    float cdz = e.coverPoint.z - e.position.z;
    float cd = sqrtf(cdx * cdx + cdz * cdz);
    if (cd > 0.5f) {
      e.position.x += (cdx / cd) * e.speed * dt;
      e.position.z += (cdz / cd) * e.speed * dt;
    }
  }

  // Floor
  if (e.position.y < 0)
    e.position.y = 0;
}

void EnemyManagerInit(EnemyManager &em, const std::vector<Vector3> &spawnPts,
                      int count, float hMult, float sMult, float aMult) {
  em.enemies.clear();
  for (int i = 0; i < count; i++) {
    Enemy e = {};
    Vector3 sp =
        spawnPts.empty() ? Vector3{0, 0, 0} : spawnPts[i % spawnPts.size()];
    EnemyInit(e, sp, GetRandomEnemyType(1), hMult, sMult, aMult);
    em.enemies.push_back(e);
  }
}

void EnemyManagerUpdate(EnemyManager &em, Vector3 playerPos, float dt) {
  for (auto &e : em.enemies) {
    if (!e.active)
      continue;
    if (e.hitFlashTimer > 0)
      e.hitFlashTimer -= dt;
    e.legAnimTimer += dt;

    if (e.state == EnemyState::DEAD) {
      e.deathAnimTimer += dt;
      if (e.deathAnimTimer > 3.0f)
        e.active = false;
      continue;
    }

    switch (e.state) {
    case EnemyState::PATROL:
      UpdatePatrol(e, playerPos, dt);
      break;
    case EnemyState::ALERT:
      UpdateAlert(e, playerPos, dt);
      break;
    case EnemyState::ATTACK:
      UpdateAttack(e, playerPos, dt);
      break;
    case EnemyState::FLANK:
      UpdateFlank(e, playerPos, dt);
      break;
    default:
      break;
    }

    if (e.fireCooldown > 0)
      e.fireCooldown -= dt;
  }
}

// ===== DRAW =====
void EnemyManagerDraw(const EnemyManager &em) {
  for (const auto &e : em.enemies) {
    if (!e.active)
      continue;

    Color bodyColor = e.hitFlashTimer > 0 ? WHITE : e.color;
    Color accentCol = e.hitFlashTimer > 0 ? WHITE : e.accentColor;
    Color bodyDark = {(unsigned char)(bodyColor.r * 0.7f),
                      (unsigned char)(bodyColor.g * 0.7f),
                      (unsigned char)(bodyColor.b * 0.7f), 255};
    Color skinColor = {210, 180, 160, 255};
    if (e.hitFlashTimer > 0)
      skinColor = WHITE;

    // ====== DOG ======
    if (e.type == EnemyType::DOG) {
      if (e.state == EnemyState::DEAD) {
        float fp = fminf(e.deathAnimTimer / 0.5f, 1.0f);
        float baseY = e.position.y + 0.1f;
        DrawSphere({e.position.x, baseY, e.position.z}, 0.2f,
                   {(unsigned char)(e.color.r * 0.4f),
                    (unsigned char)(e.color.g * 0.4f),
                    (unsigned char)(e.color.b * 0.4f), 200});
        continue;
      }
      float baseY = e.position.y;
      float anim = sinf(e.legAnimTimer * 10.0f) * 0.1f;
      // Body
      Vector3 bodyP = {e.position.x, baseY + 0.35f, e.position.z};
      DrawCylinderEx(bodyP,
                     {bodyP.x + sinf(e.yaw) * 0.5f, baseY + 0.4f,
                      bodyP.z + cosf(e.yaw) * 0.5f},
                     0.18f, 0.16f, 6, bodyColor);
      // Head
      Vector3 headP = {e.position.x + sinf(e.yaw) * 0.55f, baseY + 0.5f,
                       e.position.z + cosf(e.yaw) * 0.55f};
      DrawSphere(headP, 0.15f, bodyColor);
      // Snout
      Vector3 snoutP =
          Vector3Add(headP, {sinf(e.yaw) * 0.13f, -0.03f, cosf(e.yaw) * 0.13f});
      DrawSphere(snoutP, 0.07f, accentCol);
      // Legs
      for (int l = 0; l < 4; l++) {
        float side = (l < 2) ? -0.12f : 0.12f;
        float fwd = (l % 2 == 0) ? 0.15f : -0.15f;
        float swing = (l % 2 == 0) ? anim : -anim;
        Vector3 lt = {bodyP.x + cosf(e.yaw) * side + sinf(e.yaw) * fwd,
                      baseY + 0.35f,
                      bodyP.z - sinf(e.yaw) * side + cosf(e.yaw) * fwd};
        Vector3 lb = {lt.x, baseY + swing, lt.z};
        DrawCylinderEx(lt, lb, 0.04f, 0.035f, 4, accentCol);
      }
      // Tail
      Vector3 tailB = {e.position.x - sinf(e.yaw) * 0.55f, baseY + 0.5f,
                       e.position.z - cosf(e.yaw) * 0.55f};
      Vector3 tailT = {tailB.x - sinf(e.yaw) * 0.2f,
                       tailB.y + 0.25f + sinf(e.strafePhase * 3) * 0.1f,
                       tailB.z - cosf(e.yaw) * 0.2f};
      DrawCylinderEx(tailB, tailT, 0.04f, 0.01f, 4, bodyDark);
      // Health bar
      if (e.health < e.maxHealth) {
        float hp = e.health / e.maxHealth;
        Vector3 bp = {e.position.x, e.position.y + 1.2f, e.position.z};
        DrawCube(bp, 0.6f, 0.05f, 0.02f, {40, 40, 40, 200});
        Color hc = hp > 0.5f    ? (Color){50, 200, 80, 255}
                   : hp > 0.25f ? (Color){220, 180, 40, 255}
                                : (Color){220, 50, 50, 255};
        DrawCube({bp.x - 0.3f * (1.0f - hp), bp.y, bp.z}, 0.6f * hp, 0.05f,
                 0.025f, hc);
      }
      continue;
    }

    // ====== HUMANOID ======
    if (e.state == EnemyState::DEAD) {
      float fp = fminf(e.deathAnimTimer / 0.5f, 1.0f);
      Color dc = {(unsigned char)(e.color.r * 0.4f),
                  (unsigned char)(e.color.g * 0.4f),
                  (unsigned char)(e.color.b * 0.4f), 200};
      Vector3 bP = {e.position.x, e.position.y + 0.15f + 0.6f * (1.0f - fp),
                    e.position.z};
      DrawCylinder(bP, 0.25f, 0.2f, 0.3f + fp * 0.4f, 8, dc);
      DrawSphere({e.position.x + 0.3f * fp, e.position.y + 0.12f,
                  e.position.z + 0.2f * fp},
                 0.14f, dc);
      continue;
    }

    float baseY = e.position.y;
    float fa = e.yaw;
    float legAnim = sinf(e.legAnimTimer * 5.0f) * 0.3f;
    float armAnim = sinf(e.legAnimTimer * 5.0f) * 0.2f;
    bool isMoving = e.state != EnemyState::ATTACK ||
                    e.type == EnemyType::RUSHER || e.type == EnemyType::DOG;

    float lLO = isMoving ? legAnim : 0;
    float rLO = isMoving ? -legAnim : 0;

    // Boots
    DrawCylinder(
        {e.position.x - 0.12f, baseY + 0.06f, e.position.z + lLO * 0.5f}, 0.07f,
        0.06f, 0.12f, 6, bodyDark);
    DrawCylinder(
        {e.position.x + 0.12f, baseY + 0.06f, e.position.z + rLO * 0.5f}, 0.07f,
        0.06f, 0.12f, 6, bodyDark);

    // Shins
    Vector3 lSB = {e.position.x - 0.12f, baseY + 0.12f,
                   e.position.z + lLO * 0.4f};
    Vector3 lST = {e.position.x - 0.12f, baseY + 0.45f,
                   e.position.z + lLO * 0.2f};
    DrawCylinderEx(lSB, lST, 0.06f, 0.055f, 6, accentCol);
    Vector3 rSB = {e.position.x + 0.12f, baseY + 0.12f,
                   e.position.z + rLO * 0.4f};
    Vector3 rST = {e.position.x + 0.12f, baseY + 0.45f,
                   e.position.z + rLO * 0.2f};
    DrawCylinderEx(rSB, rST, 0.06f, 0.055f, 6, accentCol);
    DrawSphere(lST, 0.06f, bodyDark);
    DrawSphere(rST, 0.06f, bodyDark);

    // Thighs
    Vector3 lTT = {e.position.x - 0.12f, baseY + 0.75f, e.position.z};
    Vector3 rTT = {e.position.x + 0.12f, baseY + 0.75f, e.position.z};
    DrawCylinderEx(lST, lTT, 0.065f, 0.07f, 6, accentCol);
    DrawCylinderEx(rST, rTT, 0.065f, 0.07f, 6, accentCol);
    DrawSphere({e.position.x, baseY + 0.75f, e.position.z}, 0.12f, bodyDark);

    // Torso
    DrawCylinderEx({e.position.x, baseY + 0.78f, e.position.z},
                   {e.position.x, baseY + 1.28f, e.position.z}, 0.22f, 0.18f, 8,
                   bodyColor);

    // Shoulders
    Vector3 lSh = {e.position.x - 0.25f, baseY + 1.25f, e.position.z};
    Vector3 rSh = {e.position.x + 0.25f, baseY + 1.25f, e.position.z};
    DrawSphere(lSh, 0.08f, accentCol);
    DrawSphere(rSh, 0.08f, accentCol);

    // Arms
    float lAS = isMoving ? -armAnim : 0;
    float rAS = -0.3f;
    Vector3 lEl = {e.position.x - 0.3f, baseY + 0.95f, e.position.z + lAS};
    Vector3 rEl = {e.position.x + 0.3f, baseY + 0.95f, e.position.z + rAS};
    DrawCylinderEx(lSh, lEl, 0.06f, 0.05f, 6, bodyColor);
    DrawCylinderEx(rSh, rEl, 0.06f, 0.05f, 6, bodyColor);
    DrawSphere(lEl, 0.05f, bodyDark);
    DrawSphere(rEl, 0.05f, bodyDark);

    // Forearms / hands
    Vector3 lHand = {e.position.x - 0.32f, baseY + 0.7f,
                     e.position.z + lAS * 1.2f};
    Vector3 rHand = {e.position.x + 0.2f + sinf(fa) * 0.3f, baseY + 1.0f,
                     e.position.z + cosf(fa) * 0.3f};
    DrawCylinderEx(lEl, lHand, 0.045f, 0.04f, 6, skinColor);
    DrawCylinderEx(rEl, rHand, 0.045f, 0.04f, 6, skinColor);
    DrawSphere(lHand, 0.04f, skinColor);
    DrawSphere(rHand, 0.04f, skinColor);

    // Neck + Head
    DrawCylinderEx({e.position.x, baseY + 1.28f, e.position.z},
                   {e.position.x, baseY + 1.38f, e.position.z}, 0.06f, 0.07f, 6,
                   skinColor);
    Vector3 headP = {e.position.x, baseY + 1.48f, e.position.z};
    DrawSphere(headP, 0.14f, skinColor);
    // Eyes
    DrawSphere({headP.x - 0.05f + sinf(fa) * 0.12f, headP.y + 0.02f,
                headP.z + cosf(fa) * 0.12f},
               0.02f, BLACK);
    DrawSphere({headP.x + 0.05f + sinf(fa) * 0.12f, headP.y + 0.02f,
                headP.z + cosf(fa) * 0.12f},
               0.02f, BLACK);

    // Type-specific gear
    if (e.type == EnemyType::HEAVY) {
      DrawSphere({e.position.x, baseY + 1.58f, e.position.z}, 0.17f,
                 {50, 55, 50, 255});
      DrawCylinder({e.position.x, baseY + 1.58f, e.position.z}, 0.18f, 0.16f,
                   0.04f, 10, {45, 48, 45, 255});
      DrawCylinder({e.position.x + sinf(fa) * 0.2f, baseY + 1.05f,
                    e.position.z + cosf(fa) * 0.2f},
                   0.15f, 0.13f, 0.35f, 8, {70, 75, 70, 200});
      DrawCylinder(lSh, 0.1f, 0.08f, 0.06f, 6, {60, 65, 60, 255});
      DrawCylinder(rSh, 0.1f, 0.08f, 0.06f, 6, {60, 65, 60, 255});
    }
    if (e.type == EnemyType::RUSHER) {
      DrawCylinder({e.position.x, baseY + 1.58f, e.position.z}, 0.15f, 0.02f,
                   0.06f, 8, {40, 130, 40, 255});
      DrawCylinder({e.position.x + sinf(fa) * 0.1f, baseY + 1.4f,
                    e.position.z + cosf(fa) * 0.1f},
                   0.08f, 0.1f, 0.06f, 6, {30, 100, 30, 220});
    }
    if (e.type == EnemyType::SOLDIER) {
      DrawCylinder({e.position.x, baseY + 1.58f, e.position.z}, 0.13f, 0.12f,
                   0.05f, 8, bodyDark);
    }
    if (e.type == EnemyType::SNIPER) {
      // Ghillie-style hood (dark green sphere over head)
      DrawSphere({e.position.x, baseY + 1.55f, e.position.z}, 0.19f,
                 {35, 60, 35, 220});
      // Sniper rifle — extra long
      Vector3 gDir = {sinf(fa), 0, cosf(fa)};
      Vector3 gBase = rHand;
      Vector3 gEnd = Vector3Add(gBase, Vector3Scale(gDir, 1.1f));
      DrawCylinderEx(gBase, gEnd, 0.018f, 0.012f, 6, {45, 45, 50, 255});
      // Scope
      Vector3 scopeS = Vector3Add(gBase, Vector3Scale(gDir, 0.2f));
      scopeS.y += 0.025f;
      Vector3 scopeE = Vector3Add(scopeS, Vector3Scale(gDir, 0.15f));
      DrawCylinderEx(scopeS, scopeE, 0.015f, 0.015f, 6, {25, 25, 30, 255});
      // Sniper laser dot indicator
      DrawSphere(Vector3Add(gEnd, Vector3Scale(gDir, 0.05f)), 0.01f, RED);
    } else {
      // Standard gun
      Vector3 gDir = {sinf(fa), 0, cosf(fa)};
      Vector3 gEnd = Vector3Add(rHand, Vector3Scale(gDir, 0.7f));
      DrawCylinderEx(rHand, gEnd, 0.02f, 0.015f, 6, {55, 55, 60, 255});
      Vector3 recvP = Vector3Add(rHand, Vector3Scale(gDir, 0.1f));
      recvP.y += 0.03f;
      DrawCube(recvP, 0.05f, 0.06f, 0.15f, {65, 65, 70, 255});
      DrawCube(Vector3Add(recvP, {0, -0.06f, 0}), 0.025f, 0.07f, 0.04f,
               {50, 50, 55, 255});
    }

    // Health bar
    if (e.health < e.maxHealth) {
      float hp = e.health / e.maxHealth;
      Vector3 bp = {e.position.x, baseY + e.height + 0.3f, e.position.z};
      DrawCube(bp, 0.8f, 0.06f, 0.02f, {40, 40, 40, 200});
      Color hc = hp > 0.5f    ? (Color){50, 200, 80, 255}
                 : hp > 0.25f ? (Color){220, 180, 40, 255}
                              : (Color){220, 50, 50, 255};
      DrawCube({bp.x - 0.4f * (1.0f - hp), bp.y, bp.z}, 0.8f * hp, 0.06f,
               0.025f, hc);
    }
  }
}

int EnemyCheckHits(EnemyManager &em, Ray ray, float damage, float range,
                   float accuracy) {
  int hits = 0;
  float best = range;
  Enemy *bestEnemy = nullptr;
  bool bestHeadshot = false;

  for (auto &e : em.enemies) {
    if (!e.active || e.state == EnemyState::DEAD)
      continue;
    BoundingBox bb = {
        {e.position.x - e.radius, e.position.y, e.position.z - e.radius},
        {e.position.x + e.radius, e.position.y + e.height,
         e.position.z + e.radius}};
    RayCollision rc = GetRayCollisionBox(ray, bb);
    if (rc.hit && rc.distance < best) {
      best = rc.distance;
      bestEnemy = &e;
      // Headshot: top 15% of bbox
      bestHeadshot = (rc.point.y > e.position.y + e.height * 0.85f);
    }
  }

  if (bestEnemy) {
    float acc =
        accuracy + ((float)rand() / RAND_MAX - 0.5f) * (1.0f - accuracy);
    if ((float)rand() / RAND_MAX < acc) {
      float dmg = bestHeadshot ? damage * 2.5f : damage;
      EnemyApplyDamage(*bestEnemy, dmg);
      hits = bestHeadshot ? 2 : 1;
    }
  }
  return hits;
}

void EnemyApplyDamage(Enemy &e, float damage) {
  if (e.state == EnemyState::DEAD)
    return;
  e.health -= damage;
  e.hitFlashTimer = 0.12f;
  if (e.state == EnemyState::PATROL)
    e.state = EnemyState::ALERT;
  if (e.health <= 0) {
    e.health = 0;
    e.state = EnemyState::DEAD;
    e.deathAnimTimer = 0;
  }
}

bool EnemyCanSeePlayer(const Enemy &e, Vector3 playerPos) {
  float dx = playerPos.x - e.position.x;
  float dz = playerPos.z - e.position.z;
  return sqrtf(dx * dx + dz * dz) < e.detectionRange;
}

int EnemyCountAlive(const EnemyManager &em) {
  int c = 0;
  for (const auto &e : em.enemies)
    if (e.active && e.state != EnemyState::DEAD)
      c++;
  return c;
}

bool EnemyIsMelee(const EnemyType type) { return type == EnemyType::DOG; }
