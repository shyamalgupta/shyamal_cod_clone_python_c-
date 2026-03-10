#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

enum class EnemyState { PATROL, ALERT, ATTACK, DEAD, FLANK };

enum class EnemyType {
  SOLDIER, // balanced — red
  RUSHER,  // fast, low HP — green
  HEAVY,   // slow, tanky — dark gray
  SNIPER,  // long range, accurate — dark blue
  DOG      // very fast, melee only — brown
};

struct Enemy {
  Vector3 position;
  Vector3 velocity;
  float yaw;
  float health;
  float maxHealth;
  float speed;
  float radius;
  float height;
  EnemyState state;
  EnemyType type;

  // AI parameters
  float detectionRange;
  float attackRange;
  float accuracy;
  float reactionTime;
  float reactionTimer;
  float fireRate;
  float fireCooldown;
  float damage;

  // Patrol
  Vector3 patrolTarget;
  float patrolWaitTimer;

  // Flanking (new!)
  Vector3 flankTarget;
  float flankTimer;

  // Cover (new!)
  Vector3 coverPoint;
  bool inCover;
  float coverTimer;

  // Death
  float deathTimer;
  float respawnTime;
  Vector3 spawnPoint;
  float deathAnimTimer;

  // Visual
  Color color;
  Color accentColor;
  float hitFlashTimer;
  bool active;

  // Strafing / animation
  float strafePhase;
  float strafeSpeed;
  float legAnimTimer;

  // Dog-specific
  float biteTimer;
  float biteCooldown;
};

struct EnemyManager {
  std::vector<Enemy> enemies;
  int totalKills;
};

void EnemyManagerInit(EnemyManager &em, const std::vector<Vector3> &spawnPoints,
                      int count, float healthMult = 1.0f,
                      float speedMult = 1.0f, float accMult = 1.0f);
void EnemyManagerUpdate(EnemyManager &em, Vector3 playerPos, float dt);
void EnemyManagerDraw(const EnemyManager &em);
int EnemyCheckHits(EnemyManager &em, Ray ray, float weaponDamage,
                   float weaponRange, float accuracy);
void EnemyApplyDamage(Enemy &enemy, float damage);
bool EnemyCanSeePlayer(const Enemy &enemy, Vector3 playerPos);
int EnemyCountAlive(const EnemyManager &em);
bool EnemyIsMelee(const EnemyType type);
