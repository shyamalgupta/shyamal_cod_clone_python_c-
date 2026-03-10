/***********************************************************************
 *  COD FPS — Call of Duty-Style First Person Shooter (MAJOR UPGRADE)
 *  Built with C++ (Raylib)
 *
 *  Controls:
 *    WASD          — Move              Mouse — Look
 *    Left Click    — Shoot             Right Click — Aim (zoom)
 *    1/2/3/4/5     — Switch weapons (M4A1 / SPAS-12 / Barrett / M249 / RPG-7)
 *    V             — Knife melee (instant kill in range)
 *    K             — Activate killstreak
 *    R             — Reload            G — Throw grenade
 *    C / L-Ctrl    — Crouch            Z — Prone
 *    Shift         — Sprint (stamina)  Space — Jump
 *    Enter         — Start / Respawn   ESC — Pause / Quit
 ***********************************************************************/

#include "audio_manager.h"
#include "enemy.h"
#include "grenade.h"
#include "hud.h"
#include "killstreak.h"
#include "level.h"
#include "particles.h"
#include "physics.h"
#include "player.h"
#include "raylib.h"
#include "raymath.h"
#include "wave_system.h"
#include "weapon.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Color lerp helper (matches level.cpp)
static inline Color LerpColor(Color a, Color b, float t) {
  return {(unsigned char)(a.r + (b.r - a.r) * t),
          (unsigned char)(a.g + (b.g - a.g) * t),
          (unsigned char)(a.b + (b.b - a.b) * t), 255};
}

// Tracer round
struct Tracer {
  Vector3 start;
  Vector3 end;
  float life;
  Color color;
};

// Shell casing particle
struct ShellCasing {
  Vector3 position;
  Vector3 velocity;
  float life;
  bool active;
};

// RPG rocket projectile
struct Rocket {
  Vector3 position;
  Vector3 velocity;
  float life; // max flight time
  bool active;
  float blastRadius;
  float damage;
};

// Game states
enum class GameMode { MENU, PLAYING, GAME_OVER };

// Game state
struct GameState {
  Player player;
  WeaponSystem weapons;
  EnemyManager enemies;
  LevelData level;
  HUD hud;
  ParticleSystem particles;
  GrenadeSystem grenades;
  WaveSystem waves;
  KillstreakSystem killstreaks;

  GameMode mode;
  bool isPaused;
  bool isAiming;
  float footstepTimer;
  float heartbeatTimer;
  int screenWidth;
  int screenHeight;
  std::vector<BoundingBox> colliders;

  // Tracers
  std::vector<Tracer> tracers;

  // Shell casings
  ShellCasing shellCasings[50];

  // Airstrike tracking
  float airstrikeNextExplosion;

  // Menu animation
  float menuTimer;

  // RPG rockets
  std::vector<Rocket> rockets;
};

static GameState game;

static void SpawnShellCasing(Vector3 pos, Vector3 right, Vector3 up) {
  for (int i = 0; i < 50; i++) {
    if (!game.shellCasings[i].active) {
      game.shellCasings[i].active = true;
      game.shellCasings[i].position = pos;
      game.shellCasings[i].velocity = {
          right.x * 3.0f + ((float)rand() / RAND_MAX - 0.5f) * 2.0f,
          up.y * 2.0f + (float)rand() / RAND_MAX * 3.0f,
          right.z * 3.0f + ((float)rand() / RAND_MAX - 0.5f) * 2.0f};
      game.shellCasings[i].life = 0.8f;
      return;
    }
  }
}

static void StartNewGame() {
  srand((unsigned int)time(NULL));

  game.isPaused = false;
  game.isAiming = false;
  game.footstepTimer = 0;
  game.heartbeatTimer = 0;
  game.menuTimer = 0;
  game.airstrikeNextExplosion = 0;

  // Load level
  LevelLoadDefault(game.level);

  // Initialize subsystems
  PlayerInit(game.player, game.level.playerSpawn);
  WeaponSystemInit(game.weapons);
  HUDInit(game.hud);
  ParticleSystemInit(game.particles, 4000);
  GrenadeSystemInit(game.grenades);
  WaveSystemInit(game.waves);
  KillstreakInit(game.killstreaks);

  // Initialize enemies for wave 1 (empty — wave system spawns them)
  EnemyManagerInit(game.enemies, game.level.enemySpawns, 0);

  // Build collision list
  game.colliders = LevelGetAllColliders(game.level);

  // Clear tracers, casings, rockets
  game.tracers.clear();
  game.rockets.clear();
  for (int i = 0; i < 50; i++)
    game.shellCasings[i].active = false;

  game.mode = GameMode::PLAYING;
  DisableCursor();
}

static void SpawnWaveEnemies() {
  int count = WaveGetEnemyCount(game.waves);
  float healthMult = game.waves.enemyHealthMult;
  float speedMult = game.waves.enemySpeedMult;
  float accMult = game.waves.enemyAccuracyMult;
  EnemyManagerInit(game.enemies, game.level.enemySpawns, count, healthMult,
                   speedMult, accMult);
}

static void GameUpdate(float dt) {
  // === MENU STATE ===
  if (game.mode == GameMode::MENU) {
    game.menuTimer += dt;
    if (IsKeyPressed(KEY_ENTER)) {
      StartNewGame();
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
      CloseWindow();
    }
    return;
  }

  // === PAUSE ===
  if (IsKeyPressed(KEY_ESCAPE)) {
    game.isPaused = !game.isPaused;
  }
  if (game.isPaused)
    return;

  // === WAVE SYSTEM ===
  int aliveEnemies = EnemyCountAlive(game.enemies);
  WaveState prevState = game.waves.state;
  WaveSystemUpdate(game.waves, aliveEnemies, dt);

  // Detect wave state transitions
  if (prevState != game.waves.state) {
    if (game.waves.state == WaveState::ACTIVE) {
      // New wave started — spawn enemies
      SpawnWaveEnemies();
      game.colliders = LevelGetAllColliders(game.level); // refresh
    }
    if (game.waves.state == WaveState::COMPLETE) {
      AudioPlayWaveComplete();
      // Bonus rewards between waves
      game.grenades.playerGrenades =
          std::min(game.grenades.playerGrenades + 2, game.grenades.maxGrenades);
      for (int i = 0; i < 3; i++) {
        game.weapons.weapons[i].reserveAmmo += 30;
      }
      HUDShowNotification(game.hud, "+2 GRENADES  +AMMO", {100, 255, 100, 255});
    }
  }

  // === KILLSTREAK SYSTEM ===
  KillstreakUpdate(game.killstreaks, dt);

  // Juggernaut effect on player
  if (game.killstreaks.juggernautActive) {
    game.player.maxHealth = 200.0f;
    if (game.player.health > 0)
      game.player.speed = 10.0f;
  } else {
    game.player.maxHealth = 100.0f;
    if (game.player.health > game.player.maxHealth)
      game.player.health = game.player.maxHealth;
    game.player.speed = 8.0f;
  }

  // Airstrike effect
  if (game.killstreaks.airstrikeActive) {
    game.airstrikeNextExplosion -= dt;
    if (game.airstrikeNextExplosion <= 0) {
      // Random explosion near airstrike center
      float ax = game.killstreaks.airstrikeX + (float)(rand() % 20 - 10);
      float az = game.killstreaks.airstrikeZ + (float)(rand() % 20 - 10);
      Vector3 explosionPos = {ax, 0.0f, az};

      ParticleSpawnExplosion(game.particles, explosionPos);
      AudioPlayExplosion();
      PlayerApplyScreenShake(game.player, 0.05f, 0.3f);

      // Damage enemies in radius
      for (auto &e : game.enemies.enemies) {
        if (!e.active || e.state == EnemyState::DEAD)
          continue;
        float dist = Vector3Distance(e.position, explosionPos);
        if (dist < 8.0f) {
          float dmg = 200.0f * (1.0f - dist / 8.0f);
          bool wasDead = (e.health <= 0);
          EnemyApplyDamage(e, dmg);
          if (!wasDead && e.health <= 0) {
            game.enemies.totalKills++;
            game.player.kills++;
            WaveAddKillScore(game.waves, false);
            KillstreakOnKill(game.killstreaks);
            HUDAddKillFeed(game.hud, "Airstrike kill!", {255, 150, 50, 255});
          }
        }
      }
      game.airstrikeNextExplosion = 0.3f + (float)(rand() % 100) / 300.0f;
    }
  }

  // === LEVEL UPDATE (day/night + weather) ===
  LevelUpdate(game.level, dt);

  // Rain particles
  if (game.level.rainIntensity > 0.05f)
    ParticleSpawnRain(game.particles, game.player.position,
                      game.level.rainIntensity);

  // Rain audio
  if (game.level.rainIntensity > 0.3f)
    AudioPlayRain();

  // === AIMING ===
  game.isAiming = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
  // Sniper has narrower ADS FOV; RPG cannot aim
  float adsFOV = (game.weapons.currentWeapon == 2)   ? 20.0f
                 : (game.weapons.currentWeapon == 4) ? 75.0f
                                                     : 40.0f;
  float targetFOV = game.isAiming ? adsFOV : 75.0f;
  game.player.camera.fovy += (targetFOV - game.player.camera.fovy) * dt * 10.0f;

  // === UPDATE PLAYER ===
  if (PlayerIsAlive(game.player)) {
    Vector3 prevPos = game.player.position;
    PlayerUpdate(game.player, dt);

    // Collision resolution
    game.player.position = PhysicsSlideMove(prevPos, game.player.position,
                                            game.player.radius, game.colliders);
    // Re-sync camera after collision fix
    float camY = game.player.position.y + game.player.currentHeight +
                 game.player.bobAmount;
    game.player.camera.position = {
        game.player.position.x + game.player.shakeOffset.x,
        camY + game.player.shakeOffset.y,
        game.player.position.z + game.player.shakeOffset.z};
    Vector3 lookDir = {cosf(game.player.pitch) * sinf(game.player.yaw),
                       sinf(game.player.pitch),
                       cosf(game.player.pitch) * cosf(game.player.yaw)};
    game.player.camera.target =
        Vector3Add(game.player.camera.position, lookDir);

    // Boundary clamp
    float boundary = 60.0f;
    if (game.player.position.x > boundary)
      game.player.position.x = boundary;
    if (game.player.position.x < -boundary)
      game.player.position.x = -boundary;
    if (game.player.position.z > boundary)
      game.player.position.z = boundary;
    if (game.player.position.z < -boundary)
      game.player.position.z = -boundary;

    // === WEAPON SYSTEM ===
    WeaponSystemUpdate(game.weapons, dt);

    // === WEAPON SWITCHING (1-5) ===
    if (IsKeyPressed(KEY_ONE))
      WeaponSwitch(game.weapons, 0);
    if (IsKeyPressed(KEY_TWO))
      WeaponSwitch(game.weapons, 1);
    if (IsKeyPressed(KEY_THREE))
      WeaponSwitch(game.weapons, 2);
    if (IsKeyPressed(KEY_FOUR))
      WeaponSwitch(game.weapons, 3); // M249 LMG
    if (IsKeyPressed(KEY_FIVE))
      WeaponSwitch(game.weapons, 4); // RPG-7
    if (IsKeyPressed(KEY_SIX))
      WeaponSwitch(game.weapons, 6); // Axe
    if (IsKeyPressed(KEY_SEVEN))
      WeaponSwitch(game.weapons, 7); // Pistol

    // === RELOAD (R key) ===
    if (IsKeyPressed(KEY_R)) {
      WeaponReload(game.weapons);
    }

    // Scroll wheel switching
    float scroll = GetMouseWheelMove();
    if (scroll > 0) {
      int next = (game.weapons.currentWeapon + 1);
      if (next == 5)
        next = 6; // skip knife
      if (next > 7)
        next = 0; // max is 7
      WeaponSwitch(game.weapons, next);
    } else if (scroll < 0) {
      int prev = (game.weapons.currentWeapon - 1);
      if (prev == 5)
        prev = 4; // skip knife
      if (prev < 0)
        prev = 7;
      WeaponSwitch(game.weapons, prev);
    }

    // === KNIFE MELEE (V key) ===
    if (IsKeyPressed(KEY_V) && WeaponKnifeAttack(game.weapons)) {
      AudioPlayKnifeSwipe();
      // Raycast for melee range (2m)
      Vector3 knifePos = game.player.camera.position;
      Vector3 knifeFwd = Vector3Normalize(
          Vector3Subtract(game.player.camera.target, knifePos));
      for (auto &e : game.enemies.enemies) {
        if (!e.active || e.state == EnemyState::DEAD)
          continue;
        float d = Vector3Distance(knifePos, e.position);
        if (d < 2.2f) {
          bool wasDead = (e.health <= 0);
          EnemyApplyDamage(e, 999.0f); // instant kill
          AudioPlayKnifeHit();
          ParticleSpawnBlood(game.particles, e.position);
          HUDTriggerHitmarker(game.hud);
          if (!wasDead && e.health <= 0) {
            game.enemies.totalKills++;
            game.player.kills++;
            PlayerAddXP(game.player, 150, "Knife Kill");
            WaveAddKillScore(game.waves, false);
            KillstreakOnKill(game.killstreaks);
            HUDAddKillFeed(game.hud, "*** KNIFE KILL! ***",
                           {255, 220, 50, 255});
          }
          break;
        }
      }
    }

    // === GRENADE THROW ===
    if (IsKeyPressed(KEY_G) && game.grenades.playerGrenades > 0) {
      Vector3 camPos = game.player.camera.position;
      Vector3 camDir =
          Vector3Normalize(Vector3Subtract(game.player.camera.target, camPos));
      GrenadeThrow(game.grenades, camPos, camDir, 18.0f);
      AudioPlayGrenadePin();
    }

    // === KILLSTREAK ACTIVATE (K key) ===
    if (IsKeyPressed(KEY_K)) {
      if (KillstreakActivate(game.killstreaks)) {
        AudioPlayKillstreak();
        KillstreakType activated = KillstreakType::NONE;
        for (int i = 0; i < 3; i++) {
          if (game.killstreaks.rewards[i].activated)
            activated = game.killstreaks.rewards[i].type;
        }
        switch (activated) {
        case KillstreakType::UAV:
          HUDShowNotification(game.hud,
                              "UAV ONLINE — Enemy positions revealed!",
                              {100, 200, 255, 255});
          break;
        case KillstreakType::AIRSTRIKE:
          HUDShowNotification(game.hud, "AIRSTRIKE INBOUND!",
                              {255, 150, 50, 255});
          game.airstrikeNextExplosion = 0.5f;
          break;
        case KillstreakType::JUGGERNAUT:
          HUDShowNotification(game.hud,
                              "JUGGERNAUT ACTIVATED — Double health & speed!",
                              {255, 80, 80, 255});
          game.player.health = 200.0f;
          break;
        default:
          break;
        }
      }
    }

    // === SHOOTING ===
    bool wantShoot = false;
    Weapon &curWeapon = WeaponGetCurrent(game.weapons);
    if (curWeapon.stats.isAutomatic) {
      wantShoot = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    } else {
      wantShoot = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }

    if (wantShoot && WeaponFire(game.weapons)) {
      AudioPlayGunshot(game.weapons.currentWeapon);
      PlayerApplyScreenShake(game.player, 0.015f, 0.1f); // fire shake

      Vector3 camPos = game.player.camera.position;
      Vector3 camDir = Vector3Normalize(Vector3Subtract(
          game.player.camera.target, game.player.camera.position));
      Vector3 camRight =
          Vector3Normalize(Vector3CrossProduct(camDir, {0, 1, 0}));
      Vector3 camUp = Vector3CrossProduct(camRight, camDir);

      // Add recoil/inaccuracy
      float inaccuracy =
          (1.0f - curWeapon.stats.accuracy) * (game.isAiming ? 0.3f : 1.0f);
      if (game.player.isCrouching)
        inaccuracy *= 0.6f; // crouching = more accurate
      camDir.x += ((float)rand() / RAND_MAX - 0.5f) * inaccuracy * 0.1f;
      camDir.y += ((float)rand() / RAND_MAX - 0.5f) * inaccuracy * 0.1f;
      camDir.z += ((float)rand() / RAND_MAX - 0.5f) * inaccuracy * 0.1f;
      camDir = Vector3Normalize(camDir);

      // Muzzle flash
      Vector3 muzzlePos = Vector3Add(camPos, Vector3Scale(camDir, 1.5f));
      muzzlePos.y -= 0.3f;
      ParticleSpawnMuzzleFlash(game.particles, muzzlePos, camDir);

      // Shell casing
      Vector3 casingPos = Vector3Add(camPos, Vector3Scale(camRight, 0.3f));
      SpawnShellCasing(casingPos, camRight, camUp);

      // Tracer round
      Tracer tracer;
      tracer.start = muzzlePos;
      tracer.end =
          Vector3Add(muzzlePos, Vector3Scale(camDir, curWeapon.stats.range));
      tracer.life = 0.1f;
      // Different tracer colors per weapon
      if (game.weapons.currentWeapon == 0)
        tracer.color = {255, 220, 100, 200};
      else if (game.weapons.currentWeapon == 1)
        tracer.color = {255, 150, 50, 180};
      else
        tracer.color = {255, 100, 100, 220};

      // Shotgun: multiple pellets
      int pellets = curWeapon.stats.pelletsPerShot;
      float nearestHit = curWeapon.stats.range;

      for (int p = 0; p < pellets; p++) {
        Vector3 pelletDir = camDir;
        if (pellets > 1) {
          pelletDir.x += ((float)rand() / RAND_MAX - 0.5f) * 0.08f;
          pelletDir.y += ((float)rand() / RAND_MAX - 0.5f) * 0.06f;
          pelletDir.z += ((float)rand() / RAND_MAX - 0.5f) * 0.08f;
          pelletDir = Vector3Normalize(pelletDir);
        }

        Ray bulletRay = {camPos, pelletDir};

        // Check enemy hits
        for (auto &e : game.enemies.enemies) {
          if (!e.active || e.state == EnemyState::DEAD)
            continue;

          BoundingBox bodyBox = {
              {e.position.x - 0.3f, e.position.y, e.position.z - 0.2f},
              {e.position.x + 0.3f, e.position.y + e.height,
               e.position.z + 0.2f}};

          RayCollision hit = GetRayCollisionBox(bulletRay, bodyBox);
          if (hit.hit && hit.distance < curWeapon.stats.range) {
            float dmg = curWeapon.stats.damage;
            bool headshot = false;
            if (hit.point.y > e.position.y + e.height - 0.3f) {
              dmg *= curWeapon.stats.headshotMult;
              headshot = true;
            }
            bool wasDead = (e.health <= 0);
            EnemyApplyDamage(e, dmg);
            ParticleSpawnBlood(game.particles, hit.point);
            HUDTriggerHitmarker(game.hud);
            AudioPlayHitmarker();

            if (hit.distance < nearestHit)
              nearestHit = hit.distance;

            if (!wasDead && e.health <= 0) {
              game.enemies.totalKills++;
              game.player.kills++;

              // XP gain
              int xpGain = headshot ? 150 : 100;
              if (e.type == EnemyType::HEAVY)
                xpGain += 50;
              if (e.type == EnemyType::SNIPER)
                xpGain += 75;
              if (e.type == EnemyType::DOG)
                xpGain += 25;
              const char *killReason = headshot ? "Headshot Kill" : "Kill";
              PlayerAddXP(game.player, xpGain, killReason);

              // Rank-up notification
              if (game.player.rankUpPending) {
                char rankMsg[64];
                snprintf(rankMsg, sizeof(rankMsg), "RANK UP! -> %s",
                         PlayerGetRankName(game.player));
                HUDShowNotification(game.hud, rankMsg, {255, 215, 0, 255});
                AudioPlayRankUp();
                game.player.rankUpPending = false;
              }

              WaveAddKillScore(game.waves, headshot);
              KillstreakOnKill(game.killstreaks);

              // Kill feed message
              char killMsg[64];
              if (e.type == EnemyType::DOG)
                snprintf(killMsg, sizeof(killMsg),
                         headshot ? "Dog Headshot! +%d XP" : "Dog Kill +%d XP",
                         xpGain);
              else if (e.type == EnemyType::SNIPER)
                snprintf(killMsg, sizeof(killMsg),
                         headshot ? "Sniper Headshot! +%d XP"
                                  : "Sniper Down +%d XP",
                         xpGain);
              else
                snprintf(killMsg, sizeof(killMsg),
                         headshot ? "HEADSHOT! +%d XP" : "Enemy Down +%d XP",
                         xpGain);
              Color killColor = headshot ? (Color){255, 215, 0, 255}
                                         : (Color){255, 80, 80, 255};
              HUDAddKillFeed(game.hud, killMsg, killColor);
            }
            break;
          }
        }

        // Check wall hits (for impact particles)
        for (const auto &collider : game.colliders) {
          RayCollision wallHit = GetRayCollisionBox(bulletRay, collider);
          if (wallHit.hit && wallHit.distance < curWeapon.stats.range) {
            Vector3 impactNormal = {0, 1, 0}; // approximate upward normal
            ParticleSpawnSparks(game.particles, wallHit.point, 4);
            ParticleSpawnImpact(game.particles, wallHit.point, impactNormal);
            if (wallHit.distance < nearestHit)
              nearestHit = wallHit.distance;
            break;
          }
        }
      }

      // Adjust tracer end to nearest hit
      tracer.end = Vector3Add(muzzlePos, Vector3Scale(camDir, nearestHit));
      game.tracers.push_back(tracer);

      // Apply recoil to camera
      game.player.pitch +=
          curWeapon.stats.recoilAmount * (game.isAiming ? 0.5f : 1.0f);
    }

    // === RPG ROCKET LAUNCH ===
    if (game.weapons.currentWeapon == 4 && game.weapons.justFired) {
      Vector3 rPos = game.player.camera.position;
      Vector3 rDir =
          Vector3Normalize(Vector3Subtract(game.player.camera.target, rPos));
      Rocket rocket;
      rocket.position = Vector3Add(rPos, Vector3Scale(rDir, 1.5f));
      rocket.velocity = Vector3Scale(rDir, 30.0f);
      rocket.life = 6.0f;
      rocket.active = true;
      rocket.blastRadius = 8.0f;
      rocket.damage = 200.0f;
      game.rockets.push_back(rocket);
    }

    // === FOOTSTEP SOUNDS ===
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) ||
        IsKeyDown(KEY_D)) {
      float stepRate = game.player.isSprinting
                           ? 0.3f
                           : (game.player.isCrouching ? 0.6f : 0.45f);
      game.footstepTimer -= dt;
      if (game.footstepTimer <= 0 && game.player.isGrounded) {
        AudioPlayFootstep();
        game.footstepTimer = stepRate;
      }
    } else {
      game.footstepTimer = 0;
    }

    // === LOW HEALTH HEARTBEAT ===
    if (game.player.health > 0 && game.player.health < 30.0f) {
      game.heartbeatTimer -= dt;
      if (game.heartbeatTimer <= 0) {
        AudioPlayHeartbeat();
        game.heartbeatTimer = 1.0f;
      }
    }

  } else {
    // === DEAD ===
    if (IsKeyPressed(KEY_ENTER)) {
      // Full restart
      game.mode = GameMode::MENU;
      EnableCursor();
    }
  }

  // === UPDATE GRENADES ===
  GrenadeSystemUpdate(game.grenades, dt, game.colliders);
  auto explosions = GrenadeGetExplosions(game.grenades);
  for (const auto &exp : explosions) {
    ParticleSpawnExplosion(game.particles, exp.position);
    AudioPlayExplosion();
    PlayerApplyScreenShake(game.player, 0.08f, 0.4f);

    // Damage enemies
    for (auto &e : game.enemies.enemies) {
      if (!e.active || e.state == EnemyState::DEAD)
        continue;
      float dist = Vector3Distance(e.position, exp.position);
      if (dist < exp.blastRadius) {
        float dmg = exp.damage * (1.0f - dist / exp.blastRadius);
        bool wasDead = (e.health <= 0);
        EnemyApplyDamage(e, dmg);
        if (!wasDead && e.health <= 0) {
          game.enemies.totalKills++;
          game.player.kills++;
          WaveAddKillScore(game.waves, false);
          KillstreakOnKill(game.killstreaks);
          HUDAddKillFeed(game.hud, "Grenade kill!", {255, 180, 50, 255});
        }
      }
    }

    // Damage player if too close
    float playerDist = Vector3Distance(game.player.position, exp.position);
    if (playerDist < exp.blastRadius * 0.5f) {
      float dmg =
          exp.damage * 0.5f * (1.0f - playerDist / (exp.blastRadius * 0.5f));
      PlayerApplyDamage(game.player, dmg);
    }
  }

  // === UPDATE RPG ROCKETS ===
  for (auto &r : game.rockets) {
    if (!r.active)
      continue;
    r.position = Vector3Add(r.position, Vector3Scale(r.velocity, dt));
    r.life -= dt;
    ParticleSpawnRocketTrail(game.particles, r.position);

    if (r.life <= 0) {
      r.active = false;
      continue;
    }

    // Check hits
    bool detonated = false;
    // Enemy hit
    for (auto &e : game.enemies.enemies) {
      if (!e.active || e.state == EnemyState::DEAD)
        continue;
      if (Vector3Distance(r.position, e.position) < e.radius + 0.4f) {
        detonated = true;
        break;
      }
    }
    // Wall hit
    for (const auto &col : game.colliders) {
      if (GetRayCollisionBox({r.position, Vector3Normalize(r.velocity)}, col)
              .hit &&
          Vector3Distance(r.position, col.min) < 1.5f) {
        detonated = true;
        break;
      }
    }
    // Ground hit
    if (r.position.y <= 0.1f)
      detonated = true;

    if (detonated) {
      r.active = false;
      ParticleSpawnExplosion(game.particles, r.position);
      ParticleSpawnShockwave(game.particles, r.position, r.blastRadius);
      AudioPlayExplosion();
      PlayerApplyScreenShake(game.player, 0.12f, 0.5f);
      for (auto &e : game.enemies.enemies) {
        if (!e.active || e.state == EnemyState::DEAD)
          continue;
        float d = Vector3Distance(r.position, e.position);
        if (d < r.blastRadius) {
          float dmg = r.damage * (1.0f - d / r.blastRadius);
          bool wasDead = (e.health <= 0);
          EnemyApplyDamage(e, dmg);
          if (!wasDead && e.health <= 0) {
            game.enemies.totalKills++;
            game.player.kills++;
            PlayerAddXP(game.player, 200, "RPG Kill");
            WaveAddKillScore(game.waves, false);
            KillstreakOnKill(game.killstreaks);
            HUDAddKillFeed(game.hud, "RPG Kill! +200 XP", {255, 140, 30, 255});
          }
        }
      }
      // Self-damage
      float pd = Vector3Distance(game.player.position, r.position);
      if (pd < r.blastRadius * 0.7f)
        PlayerApplyDamage(game.player,
                          r.damage * 0.5f * (1.0f - pd / r.blastRadius));
    }
  }
  game.rockets.erase(std::remove_if(game.rockets.begin(), game.rockets.end(),
                                    [](const Rocket &r) { return !r.active; }),
                     game.rockets.end());

  // === UPDATE ENEMIES ===
  EnemyManagerUpdate(game.enemies, game.player.position, dt);

  // === ENEMY SHOOTING ===
  if (PlayerIsAlive(game.player)) {
    for (auto &e : game.enemies.enemies) {
      if (!e.active ||
          (e.state != EnemyState::ATTACK && e.state != EnemyState::FLANK))
        continue;
      if (EnemyIsMelee(e.type)) {
        // Dog melee bite
        float dist = Vector3Distance(e.position, game.player.position);
        if (dist < e.attackRange && e.biteCooldown <= 0) {
          PlayerApplyDamage(game.player, e.damage);
          AudioPlayDamage();
          e.biteCooldown = 1.2f;
          HUDTriggerDamageIndicator(game.hud, e.yaw);
          PlayerApplyScreenShake(game.player, 0.05f, 0.3f);
        }
        e.biteTimer -= dt;
        e.biteCooldown -= dt;
        continue;
      }
      if (e.fireCooldown <= 0) {
        float dist = Vector3Distance(e.position, game.player.position);
        if (dist < e.attackRange) {
          float hitChance = e.accuracy * (1.0f - dist / (e.attackRange * 2.0f));
          if ((float)rand() / RAND_MAX < hitChance) {
            float dmg = e.damage;
            if (game.player.isCrouching)
              dmg *= 0.8f; // crouching reduces incoming damage
            PlayerApplyDamage(game.player, dmg);
          }
          AudioPlayDamage();

          if (!PlayerIsAlive(game.player)) {
            KillstreakOnDeath(game.killstreaks);
          }
          e.fireCooldown = 1.0f / e.fireRate;

          // Enemy muzzle flash
          Vector3 enemyDir = Vector3Normalize(
              Vector3Subtract(game.player.position, e.position));
          Vector3 enemyMuzzle = {e.position.x + enemyDir.x * 0.8f,
                                 e.position.y + 1.2f,
                                 e.position.z + enemyDir.z * 0.8f};
          ParticleSpawnMuzzleFlash(game.particles, enemyMuzzle, enemyDir);

          // Enemy tracer
          Tracer enemyTracer;
          enemyTracer.start = enemyMuzzle;
          enemyTracer.end =
              Vector3Add(enemyMuzzle, Vector3Scale(enemyDir, dist));
          enemyTracer.life = 0.08f;
          enemyTracer.color = {255, 100, 100, 150};
          game.tracers.push_back(enemyTracer);
        }
      }
    }
  }

  // === UPDATE PARTICLES ===
  ParticleSystemUpdate(game.particles, dt);

  // === UPDATE TRACERS ===
  for (auto &t : game.tracers) {
    t.life -= dt;
  }
  game.tracers.erase(
      std::remove_if(game.tracers.begin(), game.tracers.end(),
                     [](const Tracer &t) { return t.life <= 0; }),
      game.tracers.end());

  // === UPDATE SHELL CASINGS ===
  for (int i = 0; i < 50; i++) {
    if (!game.shellCasings[i].active)
      continue;
    game.shellCasings[i].position =
        Vector3Add(game.shellCasings[i].position,
                   Vector3Scale(game.shellCasings[i].velocity, dt));
    game.shellCasings[i].velocity.y -= 15.0f * dt;
    game.shellCasings[i].life -= dt;
    if (game.shellCasings[i].life <= 0 || game.shellCasings[i].position.y < 0)
      game.shellCasings[i].active = false;
  }

  // === UPDATE HUD ===
  HUDUpdate(game.hud, dt);
}

static void DrawDetailedWeapon(const GameState &g) {
  if (!PlayerIsAlive(g.player))
    return;

  Vector3 camPos = g.player.camera.position;
  Vector3 camFwd =
      Vector3Normalize(Vector3Subtract(g.player.camera.target, camPos));
  Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camFwd, {0, 1, 0}));
  Vector3 camUp = Vector3CrossProduct(camRight, camFwd);

  // Base weapon position
  float bobOffset = sinf(g.player.bobTimer * 0.5f) * 0.02f;
  float aimLerp = g.isAiming ? 0.0f : 1.0f;

  Vector3 weaponPos = camPos;
  weaponPos = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.8f));
  weaponPos = Vector3Add(weaponPos, Vector3Scale(camRight, 0.25f * aimLerp));
  weaponPos = Vector3Add(weaponPos, Vector3Scale(camUp, -0.25f + bobOffset));

  // Apply recoil
  Weapon &curW = WeaponGetCurrent(const_cast<WeaponSystem &>(g.weapons));
  weaponPos =
      Vector3Add(weaponPos, Vector3Scale(camFwd, -curW.currentRecoil * 0.3f));
  weaponPos =
      Vector3Add(weaponPos, Vector3Scale(camUp, curW.currentRecoil * 0.1f));

  switch (g.weapons.currentWeapon) {
  case 0: { // M4A1 Assault Rifle
    Color gunMetal = {55, 55, 60, 255};
    Color gunDark = {40, 40, 45, 255};
    Color gunAccent = {70, 70, 75, 255};

    // Barrel (cylinder)
    Vector3 barrelEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.3f));
    DrawCylinderEx(weaponPos, barrelEnd, 0.018f, 0.015f, 8, gunMetal);
    // Handguard around barrel
    Vector3 hgStart = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.02f));
    Vector3 hgEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.2f));
    DrawCylinderEx(hgStart, hgEnd, 0.028f, 0.025f, 6, gunAccent);
    // Upper receiver
    Vector3 receiverPos = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.1f));
    receiverPos = Vector3Add(receiverPos, Vector3Scale(camUp, 0.015f));
    DrawCube(receiverPos, 0.048f, 0.055f, 0.22f, gunAccent);
    // Lower receiver
    Vector3 lowerPos = Vector3Add(receiverPos, Vector3Scale(camUp, -0.04f));
    DrawCube(lowerPos, 0.042f, 0.035f, 0.15f, gunDark);
    // Magazine (curved)
    Vector3 magPos = Vector3Add(lowerPos, Vector3Scale(camUp, -0.055f));
    magPos = Vector3Add(magPos, Vector3Scale(camFwd, -0.02f));
    DrawCube(magPos, 0.025f, 0.075f, 0.05f, gunDark);
    // Stock (tube)
    Vector3 stockStart = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.2f));
    Vector3 stockEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.38f));
    DrawCylinderEx(stockStart, stockEnd, 0.015f, 0.02f, 6, gunMetal);
    // Buttpad
    DrawCube(stockEnd, 0.035f, 0.05f, 0.02f, {30, 30, 35, 255});
    // Grip
    Vector3 gripTop = Vector3Add(lowerPos, Vector3Scale(camFwd, 0.02f));
    Vector3 gripBot = Vector3Add(gripTop, Vector3Scale(camUp, -0.06f));
    gripBot = Vector3Add(gripBot, Vector3Scale(camFwd, -0.01f));
    DrawCylinderEx(gripTop, gripBot, 0.015f, 0.018f, 6, gunDark);
    // Muzzle (flash hider)
    Vector3 muzzleStart = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.3f));
    Vector3 muzzleEnd2 = Vector3Add(muzzleStart, Vector3Scale(camFwd, 0.035f));
    DrawCylinderEx(muzzleStart, muzzleEnd2, 0.02f, 0.012f, 8,
                   {80, 80, 85, 255});
    // Picatinny rail
    Vector3 railPos = Vector3Add(receiverPos, Vector3Scale(camUp, 0.03f));
    DrawCube(railPos, 0.03f, 0.006f, 0.2f, {65, 65, 70, 255});
    // Front sight
    Vector3 fsPos = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.22f));
    fsPos = Vector3Add(fsPos, Vector3Scale(camUp, 0.03f));
    DrawCube(fsPos, 0.003f, 0.025f, 0.003f, gunDark);
    break;
  }
  case 1: { // SPAS-12 Shotgun
    Color wood = {85, 55, 30, 255};
    Color metal = {70, 65, 60, 255};
    Color darkMetal = {50, 45, 40, 255};

    // Main barrel (cylinder)
    Vector3 barrelEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.32f));
    DrawCylinderEx(weaponPos, barrelEnd, 0.022f, 0.02f, 8, metal);
    // Under barrel (tube magazine - cylinder)
    Vector3 tubeStart = Vector3Add(weaponPos, Vector3Scale(camUp, -0.035f));
    Vector3 tubeEnd = Vector3Add(barrelEnd, Vector3Scale(camUp, -0.035f));
    tubeEnd = Vector3Add(tubeEnd, Vector3Scale(camFwd, -0.06f));
    DrawCylinderEx(tubeStart, tubeEnd, 0.02f, 0.018f, 8, darkMetal);
    // Receiver block
    Vector3 receiverPos = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.12f));
    receiverPos = Vector3Add(receiverPos, Vector3Scale(camUp, 0.005f));
    DrawCube(receiverPos, 0.058f, 0.065f, 0.14f, metal);
    // Ejection port
    Vector3 portPos = Vector3Add(receiverPos, Vector3Scale(camRight, 0.03f));
    DrawCube(portPos, 0.005f, 0.03f, 0.05f, {30, 30, 35, 255});
    // Stock (wood)
    Vector3 stockStart = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.22f));
    Vector3 stockEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.38f));
    DrawCylinderEx(stockStart, stockEnd, 0.025f, 0.03f, 6, wood);
    DrawCube(stockEnd, 0.04f, 0.06f, 0.02f, wood);
    // Pump grip (wood cylinder)
    Vector3 pumpStart = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.04f));
    pumpStart = Vector3Add(pumpStart, Vector3Scale(camUp, -0.035f));
    Vector3 pumpEnd = Vector3Add(pumpStart, Vector3Scale(camFwd, 0.1f));
    DrawCylinderEx(pumpStart, pumpEnd, 0.028f, 0.026f, 6, wood);
    // Muzzle
    Vector3 muzzleEnd2 = Vector3Add(barrelEnd, Vector3Scale(camFwd, 0.01f));
    DrawCylinderEx(barrelEnd, muzzleEnd2, 0.025f, 0.022f, 8, {85, 80, 75, 255});
    // Trigger guard (thin wire)
    Vector3 tgStart = Vector3Add(receiverPos, Vector3Scale(camUp, -0.035f));
    Vector3 tgEnd = Vector3Add(tgStart, Vector3Scale(camFwd, 0.04f));
    tgEnd = Vector3Add(tgEnd, Vector3Scale(camUp, -0.02f));
    DrawCylinderEx(tgStart, tgEnd, 0.003f, 0.003f, 4, darkMetal);
    break;
  }
  case 2: { // Barrett .50
    Color darkGreen = {35, 40, 35, 255};
    Color metal = {50, 50, 55, 255};
    Color scope = {25, 25, 30, 255};

    // Long barrel (cylinder)
    Vector3 barrelEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.4f));
    DrawCylinderEx(weaponPos, barrelEnd, 0.02f, 0.018f, 8, metal);
    // Muzzle brake (thicker cylinder at end)
    Vector3 brakeStart = barrelEnd;
    Vector3 brakeEnd2 = Vector3Add(brakeStart, Vector3Scale(camFwd, 0.05f));
    DrawCylinderEx(brakeStart, brakeEnd2, 0.03f, 0.025f, 8, {60, 60, 65, 255});
    // Brake slots (small lines)
    for (int s = 0; s < 3; s++) {
      Vector3 slotPos =
          Vector3Add(brakeStart, Vector3Scale(camFwd, -0.015f + s * 0.015f));
      DrawCube(slotPos, 0.035f, 0.005f, 0.005f, {40, 40, 45, 255});
    }
    // Upper receiver
    Vector3 receiverPos = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.08f));
    receiverPos = Vector3Add(receiverPos, Vector3Scale(camUp, 0.015f));
    DrawCube(receiverPos, 0.055f, 0.065f, 0.2f, darkGreen);
    // Lower receiver
    Vector3 lowerPos = Vector3Add(receiverPos, Vector3Scale(camUp, -0.042f));
    DrawCube(lowerPos, 0.048f, 0.03f, 0.12f, darkGreen);
    // Magazine
    Vector3 magPos = Vector3Add(lowerPos, Vector3Scale(camUp, -0.045f));
    DrawCube(magPos, 0.03f, 0.055f, 0.05f, metal);
    // Stock
    Vector3 stockStart = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.22f));
    Vector3 stockEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.38f));
    DrawCylinderEx(stockStart, stockEnd, 0.02f, 0.025f, 6, darkGreen);
    DrawCube(stockEnd, 0.04f, 0.06f, 0.03f, {30, 35, 30, 255});
    // Scope tube (cylinder!)
    Vector3 scopeStart = Vector3Add(receiverPos, Vector3Scale(camUp, 0.045f));
    scopeStart = Vector3Add(scopeStart, Vector3Scale(camFwd, -0.06f));
    Vector3 scopeEnd = Vector3Add(scopeStart, Vector3Scale(camFwd, 0.16f));
    DrawCylinderEx(scopeStart, scopeEnd, 0.018f, 0.018f, 8, scope);
    // Scope bell (front)
    Vector3 bellEnd = Vector3Add(scopeEnd, Vector3Scale(camFwd, 0.02f));
    DrawCylinderEx(scopeEnd, bellEnd, 0.025f, 0.018f, 8, scope);
    // Scope lens (front - blue tint)
    Vector3 lensFront = Vector3Add(scopeEnd, Vector3Scale(camFwd, 0.01f));
    DrawSphere(lensFront, 0.02f, {80, 140, 200, 180});
    // Scope eyepiece (back)
    Vector3 eyeStart = Vector3Add(scopeStart, Vector3Scale(camFwd, -0.02f));
    DrawCylinderEx(eyeStart, scopeStart, 0.022f, 0.015f, 8, scope);
    Vector3 lensBack = Vector3Add(scopeStart, Vector3Scale(camFwd, -0.01f));
    DrawSphere(lensBack, 0.014f, {80, 140, 200, 150});
    // Scope rings (mount to rail)
    Vector3 ring1 = Vector3Add(scopeStart, Vector3Scale(camFwd, 0.03f));
    ring1 = Vector3Add(ring1, Vector3Scale(camUp, -0.02f));
    DrawCube(ring1, 0.025f, 0.02f, 0.015f, metal);
    Vector3 ring2 = Vector3Add(scopeStart, Vector3Scale(camFwd, 0.11f));
    ring2 = Vector3Add(ring2, Vector3Scale(camUp, -0.02f));
    DrawCube(ring2, 0.025f, 0.02f, 0.015f, metal);
    // Bipod legs (cylinders)
    Vector3 bipodBase = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.15f));
    bipodBase = Vector3Add(bipodBase, Vector3Scale(camUp, -0.03f));
    Vector3 bipodL = Vector3Add(bipodBase, Vector3Scale(camRight, -0.05f));
    bipodL = Vector3Add(bipodL, Vector3Scale(camUp, -0.08f));
    Vector3 bipodR = Vector3Add(bipodBase, Vector3Scale(camRight, 0.05f));
    bipodR = Vector3Add(bipodR, Vector3Scale(camUp, -0.08f));
    DrawCylinderEx(bipodBase, bipodL, 0.005f, 0.004f, 4, metal);
    DrawCylinderEx(bipodBase, bipodR, 0.005f, 0.004f, 4, metal);
    // Bipod feet
    DrawSphere(bipodL, 0.008f, metal);
    DrawSphere(bipodR, 0.008f, metal);
    break;
  }
  case 3: { // M249 LMG
    Color metal = {45, 45, 50, 255};
    Color darkGreen = {40, 50, 40, 255};

    Vector3 barrelEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.35f));
    DrawCylinderEx(weaponPos, barrelEnd, 0.022f, 0.022f, 8, metal);
    // Handguard
    Vector3 hgStart = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.02f));
    Vector3 hgEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.18f));
    DrawCylinderEx(hgStart, hgEnd, 0.035f, 0.035f, 6, darkGreen);
    // Receiver
    Vector3 receiverPos = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.1f));
    DrawCube(receiverPos, 0.06f, 0.07f, 0.25f, metal);
    // Box Mag
    Vector3 magPos = Vector3Add(receiverPos, Vector3Scale(camRight, -0.05f));
    magPos = Vector3Add(magPos, Vector3Scale(camUp, -0.05f));
    DrawCube(magPos, 0.08f, 0.08f, 0.1f, {30, 80, 40, 255}); // green ammo box
    // Stock
    Vector3 stockStart = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.22f));
    Vector3 stockEnd = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.35f));
    DrawCylinderEx(stockStart, stockEnd, 0.02f, 0.03f, 6, metal);
    DrawCube(stockEnd, 0.04f, 0.06f, 0.02f, darkGreen);
    break;
  }
  case 4: { // RPG-7
    Color wood = {90, 60, 30, 255};
    Color metal = {50, 55, 50, 255};
    Color warhead = {60, 70, 60, 255};

    // Tube
    Vector3 tubeEndFwd = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.4f));
    Vector3 tubeEndBack = Vector3Add(weaponPos, Vector3Scale(camFwd, -0.4f));
    DrawCylinderEx(tubeEndBack, tubeEndFwd, 0.035f, 0.035f, 8, metal);
    // Trumpet exhaust
    Vector3 trumpetEnd = Vector3Add(tubeEndBack, Vector3Scale(camFwd, -0.1f));
    DrawCylinderEx(tubeEndBack, trumpetEnd, 0.035f, 0.05f, 8, metal);
    // Grips (wood)
    Vector3 grip1 = Vector3Add(weaponPos, Vector3Scale(camUp, -0.08f));
    DrawCylinderEx(weaponPos, grip1, 0.015f, 0.015f, 6, wood);
    Vector3 grip2 = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.1f));
    Vector3 grip2Bot = Vector3Add(grip2, Vector3Scale(camUp, -0.08f));
    DrawCylinderEx(grip2, grip2Bot, 0.015f, 0.015f, 6, wood);
    // Warhead (if not reloading or if we have ammo)
    if (!curW.isReloading && curW.currentAmmo > 0) {
      Vector3 wh1 = Vector3Add(tubeEndFwd, Vector3Scale(camFwd, 0.1f));
      DrawCylinderEx(tubeEndFwd, wh1, 0.035f, 0.06f, 8, warhead); // cone base
      Vector3 wh2 = Vector3Add(wh1, Vector3Scale(camFwd, 0.1f));
      DrawCylinderEx(wh1, wh2, 0.06f, 0.02f, 8, warhead); // nose
      DrawSphere(wh2, 0.02f, metal);                      // tip
    }
    break;
  }
  case 6: { // AXE
    Color handle = {70, 45, 20, 255};
    Color blade = {150, 150, 160, 255};

    // Rotate axe based on attack animation
    Vector3 axePos = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.3f));
    float swing = 0;
    if (g.weapons.knifeAttacking) {
      float t = g.weapons.knifeTimer / 0.5f; // axe timer is 0.5
      swing = sinf(t * PI) * 1.5f;           // swing forward
    }
    Vector3 handleEndTop =
        Vector3Add(axePos, Vector3Scale(camUp, 0.2f - swing * 0.2f));
    handleEndTop = Vector3Add(handleEndTop, Vector3Scale(camFwd, swing * 0.2f));
    Vector3 handleEndBot =
        Vector3Add(axePos, Vector3Scale(camUp, -0.2f - swing * 0.1f));

    DrawCylinderEx(handleEndBot, handleEndTop, 0.02f, 0.02f, 6, handle);
    // Blade
    Vector3 bladePos = Vector3Add(handleEndTop, Vector3Scale(camUp, -0.05f));
    bladePos = Vector3Add(bladePos, Vector3Scale(camRight, -0.02f));
    DrawCube(bladePos, 0.15f, 0.1f, 0.01f, blade);
    break;
  }
  case 7: { // PISTOL
    Color gunDark = {40, 40, 45, 255};

    Vector3 pistolPos = Vector3Add(weaponPos, Vector3Scale(camFwd, 0.1f));
    pistolPos = Vector3Add(pistolPos, Vector3Scale(camUp, -0.1f));
    // Slide/Barrel
    Vector3 slidePos = Vector3Add(pistolPos, Vector3Scale(camUp, 0.05f));
    DrawCube(slidePos, 0.03f, 0.03f, 0.15f, gunDark);
    // Grip
    Vector3 gripPos = Vector3Add(pistolPos, Vector3Scale(camFwd, -0.04f));
    gripPos = Vector3Add(gripPos, Vector3Scale(camUp, -0.04f));
    DrawCube(gripPos, 0.025f, 0.08f, 0.03f, {30, 25, 25, 255});
    break;
  }
  }
}

static void GameDraw() {
  BeginDrawing();

  // === MENU SCREEN ===
  if (game.mode == GameMode::MENU) {
    ClearBackground({15, 18, 22, 255});

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Animated background lines
    for (int i = 0; i < 20; i++) {
      float y = fmodf(i * 40.0f + game.menuTimer * 30.0f, (float)screenH);
      unsigned char alpha = (unsigned char)(20 + sinf(game.menuTimer + i) * 10);
      DrawLine(0, (int)y, screenW, (int)y, {60, 80, 60, alpha});
    }

    // Title
    const char *title = "COD FPS";
    int titleW = MeasureText(title, 80);
    float titlePulse = sinf(game.menuTimer * 2.0f) * 5.0f;
    DrawText(title, screenW / 2 - titleW / 2, screenH / 4 + (int)titlePulse, 80,
             {200, 220, 200, 255});

    // Subtitle
    const char *subtitle = "CALL OF DUTY STYLE SHOOTER";
    int subW = MeasureText(subtitle, 18);
    DrawText(subtitle, screenW / 2 - subW / 2, screenH / 4 + 85, 18,
             {130, 160, 130, 200});

    // Version tag
    DrawText("MEGA UPDATE v2.0", screenW / 2 - 65, screenH / 4 + 110, 14,
             {100, 200, 100, 180});

    // Start prompt (flashing)
    float flashAlpha = (sinf(game.menuTimer * 3.0f) + 1.0f) * 0.5f;
    const char *startText = "PRESS ENTER TO START";
    int startW = MeasureText(startText, 24);
    DrawText(startText, screenW / 2 - startW / 2, screenH / 2 + 40, 24,
             {200, 255, 200, (unsigned char)(100 + 155 * flashAlpha)});

    // Controls info
    int infoY = screenH / 2 + 100;
    DrawText("WASD - Move    Mouse - Look    LMB - Shoot", screenW / 2 - 200,
             infoY, 12, {120, 140, 120, 160});
    DrawText("G - Grenade    C - Crouch    Shift - Sprint    R - Reload",
             screenW / 2 - 220, infoY + 18, 12, {120, 140, 120, 160});
    DrawText("1/2/3 - Weapons    4 - Killstreak    Space - Jump",
             screenW / 2 - 200, infoY + 36, 12, {120, 140, 120, 160});

    // Quit hint
    DrawText("ESC to quit", screenW / 2 - 40, screenH - 40, 12,
             {100, 100, 100, 150});

    EndDrawing();
    return;
  }

  // Use dynamic sky color (accounts for storm darkening)
  Color skyNow = LevelGetSkyColor(game.level);
  // Fog blends sky with ground during heavy fog
  if (game.level.fogDensity > 0.3f) {
    skyNow =
        LerpColor(skyNow, game.level.fogColor, game.level.fogDensity * 0.5f);
  }
  ClearBackground(skyNow);

  // === 3D SCENE ===
  BeginMode3D(game.player.camera);

  // Level
  LevelDraw(game.level);

  // Enemies
  EnemyManagerDraw(game.enemies);

  // Grenades
  GrenadeSystemDraw(game.grenades);

  // Particles
  ParticleSystemDraw(game.particles);

  // Tracers
  for (const auto &t : game.tracers) {
    float alpha = t.life / 0.1f;
    Color c = t.color;
    c.a = (unsigned char)(c.a * alpha);
    DrawLine3D(t.start, t.end, c);
    // Thicker tracer (draw multiple times slightly offset)
    Vector3 offset = {0.005f, 0.005f, 0};
    DrawLine3D(Vector3Add(t.start, offset), Vector3Add(t.end, offset), c);
  }

  // Shell casings
  for (int i = 0; i < 50; i++) {
    if (!game.shellCasings[i].active)
      continue;
    float alpha = game.shellCasings[i].life / 0.8f;
    DrawCube(game.shellCasings[i].position, 0.015f, 0.008f, 0.025f,
             {200, 180, 80, (unsigned char)(255 * alpha)});
  }

  // Weapon model
  DrawDetailedWeapon(game);

  // RPG rockets
  for (const auto &r : game.rockets) {
    if (!r.active)
      continue;
    Vector3 rFwd = Vector3Normalize(r.velocity);
    Vector3 rEnd = Vector3Add(r.position, Vector3Scale(rFwd, 0.5f));
    DrawCylinderEx(r.position, rEnd, 0.06f, 0.04f, 6, {80, 80, 85, 255});
    DrawSphere(rEnd, 0.07f, {255, 120, 30, 255}); // nose
    // Exhaust glow
    DrawSphere(r.position, 0.12f, {255, 200, 50, 180});
  }

  EndMode3D();

  // === 2D HUD OVERLAY ===
  HUDDraw(game.hud, game.player, game.weapons, game.enemies, game.level,
          game.waves, game.killstreaks, game.grenades.playerGrenades);

  // === PAUSE SCREEN ===
  if (game.isPaused) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 180});

    const char *pauseText = "PAUSED";
    int tw = MeasureText(pauseText, 60);
    DrawText(pauseText, screenW / 2 - tw / 2, screenH / 2 - 50, 60,
             {200, 200, 200, 220});

    // Stats while paused
    char statsText[128];
    snprintf(statsText, sizeof(statsText), "Wave: %d   Score: %d   Streak: %d",
             game.waves.currentWave, game.waves.score,
             game.killstreaks.currentStreak);
    int stW = MeasureText(statsText, 16);
    DrawText(statsText, screenW / 2 - stW / 2, screenH / 2 + 10, 16,
             {180, 180, 180, 200});

    const char *resumeText = "Press ESC to resume";
    int rw = MeasureText(resumeText, 20);
    DrawText(resumeText, screenW / 2 - rw / 2, screenH / 2 + 40, 20,
             {150, 150, 150, 200});
  }

  // === SCOPE OVERLAY (sniper) ===
  if (game.isAiming && game.weapons.currentWeapon == 2 &&
      PlayerIsAlive(game.player)) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int cx = screenW / 2;
    int cy = screenH / 2;
    int scopeRadius = 200;

    DrawRectangle(0, 0, cx - scopeRadius, screenH, BLACK);
    DrawRectangle(cx + scopeRadius, 0, cx - scopeRadius, screenH, BLACK);
    DrawRectangle(cx - scopeRadius, 0, scopeRadius * 2, cy - scopeRadius,
                  BLACK);
    DrawRectangle(cx - scopeRadius, cy + scopeRadius, scopeRadius * 2,
                  cy - scopeRadius, BLACK);

    DrawLine(cx - scopeRadius, cy, cx + scopeRadius, cy, {0, 0, 0, 180});
    DrawLine(cx, cy - scopeRadius, cx, cy + scopeRadius, {0, 0, 0, 180});

    for (int i = 1; i <= 4; i++) {
      int markY = cy + i * 30;
      DrawLine(cx - 10, markY, cx + 10, markY, {0, 0, 0, 150});
    }

    DrawCircleLines(cx, cy, (float)scopeRadius, {0, 0, 0, 200});
    DrawCircleLines(cx, cy, (float)scopeRadius + 1, {0, 0, 0, 150});

    // Distance readout
    DrawText("x4.0", cx + scopeRadius - 40, cy + scopeRadius - 20, 10,
             {0, 0, 0, 150});
  }

  // === JUGGERNAUT OVERLAY ===
  if (game.killstreaks.juggernautActive) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float pulse = (sinf((float)GetTime() * 4.0f) + 1.0f) * 0.5f;
    unsigned char alpha = (unsigned char)(15 + 10 * pulse);
    DrawRectangle(0, 0, screenW, screenH, {255, 50, 30, alpha});
  }

  EndDrawing();
}

int main(void) {
  // Window setup
  game.screenWidth = 1280;
  game.screenHeight = 720;
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
  InitWindow(game.screenWidth, game.screenHeight,
             "COD FPS - Call of Duty Style Shooter [MEGA UPDATE]");
  SetTargetFPS(60);

  // Audio
  AudioManagerInit();

  // Start in menu mode
  game.mode = GameMode::MENU;
  game.menuTimer = 0;

  // Main loop
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    if (dt > 0.05f)
      dt = 0.05f; // Cap delta time

    GameUpdate(dt);
    GameDraw();
  }

  // Cleanup
  AudioManagerShutdown();
  CloseWindow();

  return 0;
}
