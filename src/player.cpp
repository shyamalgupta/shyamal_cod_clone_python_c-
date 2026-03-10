#include "player.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void PlayerInit(Player &player, Vector3 spawnPos) {
  player.position = spawnPos;
  player.velocity = {0, 0, 0};
  player.yaw = 0;
  player.pitch = 0;
  player.speed = 6.0f;
  player.sprintMultiplier = 1.6f;
  player.health = 100;
  player.maxHealth = 100;
  player.isGrounded = false;
  player.isSprinting = false;
  player.bobTimer = 0;
  player.bobAmount = 0;
  player.kills = 0;
  player.deaths = 0;
  player.damageFlashTimer = 0;
  player.standHeight = 1.8f;
  player.crouchHeight = 1.0f;
  player.proneHeight = 0.4f;
  player.height = player.standHeight;
  player.currentHeight = player.standHeight;
  player.radius = 0.3f;
  player.isCrouching = false;
  player.isProne = false;
  player.proneSpeedMult = 0.3f;
  player.stamina = 100;
  player.maxStamina = 100;
  player.staminaDrain = 30;
  player.staminaRegen = 15;
  player.staminaDepleted = false;
  player.shakeIntensity = 0;
  player.shakeTimer = 0;
  player.shakeDuration = 0;
  player.shakeOffset = {0, 0, 0};
  player.healthRegenDelay = 5.0f;
  player.healthRegenTimer = 0;
  player.healthRegenRate = 8.0f;
  player.fallStartY = spawnPos.y;
  player.wasFalling = false;
  // XP / rank
  player.xp = 0;
  player.rank = 0;
  player.rankUpPending = false;
  player.rankUpTimer = 0;
  player.xpGainDisplay = 0;
  player.xpDisplayTimer = 0;
  player.lastXpReason = "";
  // Knife
  player.knifeActive = false;
  player.knifeTimer = 0;
  player.knifeCooldown = 0;
  player.knifeAnimTimer = 0;

  // Camera
  player.camera.position = {spawnPos.x, spawnPos.y + player.standHeight,
                            spawnPos.z};
  player.camera.target = {spawnPos.x, spawnPos.y + player.standHeight,
                          spawnPos.z + 1};
  player.camera.up = {0, 1, 0};
  player.camera.fovy = 75.0f;
  player.camera.projection = CAMERA_PERSPECTIVE;
}

void PlayerUpdate(Player &player, float dt) {
  // === Input ===
  Vector2 mouseDelta = GetMouseDelta();
  float sensitivity = 0.002f;
  player.yaw -= mouseDelta.x * sensitivity;
  player.pitch += mouseDelta.y * sensitivity;
  float maxPitch = player.isProne ? 0.5f : 1.3f;
  if (player.pitch > maxPitch)
    player.pitch = maxPitch;
  if (player.pitch < -1.5f)
    player.pitch = -1.5f;

  // Forward/right vectors
  Vector3 fwd = {sinf(player.yaw), 0, cosf(player.yaw)};
  Vector3 right = {cosf(player.yaw), 0, -sinf(player.yaw)};

  // Crouch / Prone
  bool crouchKey = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_C);
  bool proneKey = IsKeyDown(KEY_Z);

  if (proneKey && player.isGrounded) {
    player.isProne = true;
    player.isCrouching = false;
  } else if (crouchKey && player.isGrounded) {
    player.isCrouching = true;
    player.isProne = false;
  } else {
    player.isCrouching = false;
    player.isProne = false;
  }

  float targetH = player.isProne       ? player.proneHeight
                  : player.isCrouching ? player.crouchHeight
                                       : player.standHeight;
  player.currentHeight += (targetH - player.currentHeight) * 10.0f * dt;
  player.height = player.currentHeight;

  // Sprint & stamina
  bool wantSprint =
      IsKeyDown(KEY_LEFT_SHIFT) && !player.isCrouching && !player.isProne;
  if (wantSprint && player.stamina > 0 && !player.staminaDepleted) {
    player.isSprinting = true;
    player.stamina -= player.staminaDrain * dt;
    if (player.stamina <= 0) {
      player.stamina = 0;
      player.staminaDepleted = true;
      player.isSprinting = false;
    }
  } else {
    player.isSprinting = false;
    player.stamina += player.staminaRegen * dt;
    if (player.stamina >= player.maxStamina) {
      player.stamina = player.maxStamina;
      player.staminaDepleted = false;
    }
    if (player.staminaDepleted && player.stamina >= player.maxStamina * 0.3f)
      player.staminaDepleted = false;
  }

  // Movement
  float spd = player.speed;
  if (player.isSprinting)
    spd *= player.sprintMultiplier;
  if (player.isCrouching)
    spd *= 0.55f;
  if (player.isProne)
    spd *= player.proneSpeedMult;

  Vector3 move = {0, 0, 0};
  if (IsKeyDown(KEY_W))
    move = Vector3Add(move, fwd);
  if (IsKeyDown(KEY_S))
    move = Vector3Subtract(move, fwd);
  if (IsKeyDown(KEY_A))
    move = Vector3Subtract(move, right);
  if (IsKeyDown(KEY_D))
    move = Vector3Add(move, right);
  if (Vector3Length(move) > 0.01f) {
    move = Vector3Normalize(move);
    move = Vector3Scale(move, spd * dt);
  }

  // Jump
  if (IsKeyPressed(KEY_SPACE) && player.isGrounded && !player.isProne) {
    player.velocity.y = 7.0f;
    player.isGrounded = false;
  }

  // Gravity
  player.velocity.y -= 22.0f * dt;
  player.position.x += move.x;
  player.position.z += move.z;
  player.position.y += player.velocity.y * dt;

  // Floor collision
  float floorY = 0.0f;
  if (player.position.y < floorY) {
    float fallDist = player.fallStartY - floorY;
    if (player.wasFalling && fallDist > 4.0f) {
      float dmg = (fallDist - 4.0f) * 10.0f;
      PlayerApplyDamage(player, dmg);
      PlayerApplyScreenShake(player, 0.05f + dmg * 0.003f, 0.35f);
    }
    player.position.y = floorY;
    player.velocity.y = 0;
    player.isGrounded = true;
    player.wasFalling = false;
    player.fallStartY = floorY;
  } else if (!player.isGrounded) {
    if (!player.wasFalling) {
      player.fallStartY = player.position.y;
      player.wasFalling = true;
    }
  }

  // Bob
  bool isMoving = Vector3Length(move) > 0.001f && player.isGrounded;
  if (isMoving) {
    float bobSpeed = player.isSprinting   ? 14.0f
                     : player.isCrouching ? 6.0f
                                          : 10.0f;
    player.bobTimer += dt * bobSpeed;
    player.bobAmount =
        sinf(player.bobTimer) * (player.isSprinting ? 0.06f : 0.03f);
  } else {
    player.bobTimer *= 0.9f;
    player.bobAmount *= 0.9f;
  }

  // Damage flash
  if (player.damageFlashTimer > 0)
    player.damageFlashTimer -= dt;

  // Health regen
  if (player.health < player.maxHealth) {
    player.healthRegenTimer += dt;
    if (player.healthRegenTimer >= player.healthRegenDelay) {
      player.health += player.healthRegenRate * dt;
      if (player.health > player.maxHealth)
        player.health = player.maxHealth;
    }
  }

  // Screen shake decay
  if (player.shakeTimer > 0) {
    player.shakeTimer -= dt;
    float t = player.shakeTimer / player.shakeDuration;
    float mag = player.shakeIntensity * t;
    float randF = (float)rand() / RAND_MAX;
    float randF2 = (float)rand() / RAND_MAX;
    player.shakeOffset = {(randF - 0.5f) * 2.0f * mag,
                          (randF2 - 0.5f) * 2.0f * mag, 0};
  } else {
    player.shakeOffset = {0, 0, 0};
  }

  // Knife timers
  if (player.knifeCooldown > 0)
    player.knifeCooldown -= dt;
  if (player.knifeTimer > 0) {
    player.knifeTimer -= dt;
    player.knifeActive = true;
  } else
    player.knifeActive = false;
  if (player.knifeAnimTimer > 0)
    player.knifeAnimTimer -= dt;

  // Rank-up display
  if (player.rankUpTimer > 0)
    player.rankUpTimer -= dt;
  if (player.xpDisplayTimer > 0)
    player.xpDisplayTimer -= dt;

  // Update camera
  float camY = player.position.y + player.height + player.bobAmount;
  camY += player.shakeOffset.y;
  player.camera.position = {player.position.x + player.shakeOffset.x, camY,
                            player.position.z + player.shakeOffset.z};

  // Look direction
  Vector3 lookDir = {sinf(player.yaw) * cosf(player.pitch), sinf(-player.pitch),
                     cosf(player.yaw) * cosf(player.pitch)};
  player.camera.target = Vector3Add(player.camera.position, lookDir);
}

void PlayerApplyDamage(Player &player, float damage) {
  if (!PlayerIsAlive(player))
    return;
  player.health -= damage;
  if (player.health < 0)
    player.health = 0;
  player.damageFlashTimer = 0.25f;
  player.healthRegenTimer = 0; // reset regen delay
  PlayerApplyScreenShake(player, damage * 0.003f, 0.25f);
}

void PlayerRespawn(Player &player, Vector3 spawnPos) {
  player.position = spawnPos;
  player.velocity = {0, 0, 0};
  player.health = player.maxHealth;
  player.deaths++;
  player.isSprinting = false;
  player.isCrouching = false;
  player.isProne = false;
  player.currentHeight = player.standHeight;
  player.height = player.standHeight;
  // XP penalty on death
  player.xp -= 50;
  if (player.xp < 0)
    player.xp = 0;
}

bool PlayerIsAlive(const Player &player) { return player.health > 0; }

Vector3 PlayerGetForward(const Player &player) {
  return {sinf(player.yaw) * cosf(player.pitch), sinf(-player.pitch),
          cosf(player.yaw) * cosf(player.pitch)};
}

Vector3 PlayerGetRight(const Player &player) {
  return {cosf(player.yaw), 0, -sinf(player.yaw)};
}

void PlayerApplyScreenShake(Player &player, float intensity, float duration) {
  if (intensity > player.shakeIntensity) {
    player.shakeIntensity = intensity;
    player.shakeDuration = duration;
    player.shakeTimer = duration;
  }
}

void PlayerAddXP(Player &player, int amount, const char *reason) {
  player.xp += amount;
  player.xpGainDisplay = amount;
  player.xpDisplayTimer = 2.0f;
  player.lastXpReason = reason;

  // Rank up check
  int newRank = player.xp / XP_PER_RANK;
  if (newRank > RANK_COUNT - 1)
    newRank = RANK_COUNT - 1;
  if (newRank > player.rank) {
    player.rank = newRank;
    player.rankUpPending = true;
    player.rankUpTimer = 3.5f;
  }
}

int PlayerGetRank(const Player &player) { return player.rank; }
const char *PlayerGetRankName(const Player &player) {
  return RANK_NAMES[player.rank < RANK_COUNT ? player.rank : RANK_COUNT - 1];
}
