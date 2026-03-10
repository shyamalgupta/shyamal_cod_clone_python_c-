#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

enum class ParticleType {
  MUZZLE_FLASH,
  IMPACT_DUST,
  BLOOD,
  EXPLOSION_DEBRIS,
  SPARK,      // new: metal bullet impact sparks
  RAIN_DROP,  // new: falling rain streaks
  SMOKE_PUFF, // new: rocket trail smoke
  SHOCKWAVE,  // new: explosion ring
  SHELL_CASE  // new: ejected shell casings
};

struct Particle {
  Vector3 position;
  Vector3 velocity;
  float life;
  float maxLife;
  float size;
  Color color;
  ParticleType type;
  bool active;
  float rotation; // for shockwave ring radius
};

struct ParticleSystem {
  std::vector<Particle> particles;
  int maxParticles;
};

void ParticleSystemInit(ParticleSystem &ps, int maxCount);
void ParticleSystemUpdate(ParticleSystem &ps, float dt);
void ParticleSystemDraw(const ParticleSystem &ps);

void ParticleSpawnMuzzleFlash(ParticleSystem &ps, Vector3 pos, Vector3 dir);
void ParticleSpawnImpact(ParticleSystem &ps, Vector3 pos, Vector3 normal);
void ParticleSpawnBlood(ParticleSystem &ps, Vector3 pos);
void ParticleSpawnExplosion(ParticleSystem &ps, Vector3 pos);
void ParticleSpawnSparks(ParticleSystem &ps, Vector3 pos, int count);
void ParticleSpawnRocketTrail(ParticleSystem &ps, Vector3 pos);
void ParticleSpawnShockwave(ParticleSystem &ps, Vector3 pos, float maxRadius);
void ParticleSpawnRain(ParticleSystem &ps, Vector3 playerPos, float intensity);
