#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>

// XP ranks
static const char *RANK_NAMES[] = {"Private",
                                   "Private II",
                                   "Private III",
                                   "Lance Corporal",
                                   "Lance Corporal II",
                                   "Corporal",
                                   "Corporal II",
                                   "Sergeant",
                                   "Sergeant II",
                                   "Staff Sergeant",
                                   "Staff Sergeant II",
                                   "Gunnery Sergeant",
                                   "Master Sergeant",
                                   "1st Sergeant",
                                   "Sergeant Major",
                                   "Warrant Officer",
                                   "Warrant Officer II",
                                   "Warrant Officer III",
                                   "Warrant Officer IV",
                                   "Warrant Officer V",
                                   "2nd Lieutenant",
                                   "1st Lieutenant",
                                   "Captain",
                                   "Major",
                                   "Lieutenant Colonel",
                                   "Colonel",
                                   "Brigadier General",
                                   "Major General",
                                   "Lieutenant General",
                                   "General",
                                   "Commander",
                                   "Commander II",
                                   "Commander III",
                                   "Commander IV",
                                   "Commander V",
                                   "Legend I",
                                   "Legend II",
                                   "Legend III",
                                   "Legend IV",
                                   "Legend V",
                                   "Elite",
                                   "Elite II",
                                   "Elite III",
                                   "Elite IV",
                                   "Elite V",
                                   "Veteran",
                                   "Veteran II",
                                   "Veteran III",
                                   "Veteran IV",
                                   "Veteran MAX"};
static const int RANK_COUNT = 50;
static const int XP_PER_RANK = 500;

struct Player {
  Camera3D camera;
  Vector3 position;
  Vector3 velocity;
  float yaw;
  float pitch;
  float speed;
  float sprintMultiplier;
  float health;
  float maxHealth;
  bool isGrounded;
  bool isSprinting;
  float bobTimer;
  float bobAmount;
  int kills;
  int deaths;
  float damageFlashTimer;
  float height;
  float radius;

  // Crouching
  bool isCrouching;
  float currentHeight;
  float standHeight;
  float crouchHeight;

  // Prone (new!)
  bool isProne;
  float proneHeight;
  float proneSpeedMult;

  // Stamina
  float stamina;
  float maxStamina;
  float staminaDrain;
  float staminaRegen;
  bool staminaDepleted;

  // Screen shake
  float shakeIntensity;
  float shakeTimer;
  float shakeDuration;
  Vector3 shakeOffset;

  // Health regen
  float healthRegenDelay;
  float healthRegenTimer;
  float healthRegenRate;

  // Landing
  float fallStartY;
  bool wasFalling;

  // XP & Rank (new!)
  int xp;
  int rank; // 0-49
  bool rankUpPending;
  float rankUpTimer;
  int xpGainDisplay;
  float xpDisplayTimer;
  std::string lastXpReason;

  // Knife melee (new!)
  bool knifeActive;
  float knifeTimer;
  float knifeCooldown;
  float knifeAnimTimer;
};

void PlayerInit(Player &player, Vector3 spawnPos);
void PlayerUpdate(Player &player, float dt);
void PlayerApplyDamage(Player &player, float damage);
void PlayerRespawn(Player &player, Vector3 spawnPos);
bool PlayerIsAlive(const Player &player);
Vector3 PlayerGetForward(const Player &player);
Vector3 PlayerGetRight(const Player &player);
void PlayerApplyScreenShake(Player &player, float intensity, float duration);
void PlayerAddXP(Player &player, int amount, const char *reason);
int PlayerGetRank(const Player &player);
const char *PlayerGetRankName(const Player &player);
