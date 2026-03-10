#include "particles.h"
#include <math.h>
#include <stdlib.h>

static float RandF(float lo, float hi) {
  return lo + (float)rand() / RAND_MAX * (hi - lo);
}

void ParticleSystemInit(ParticleSystem &ps, int maxCount) {
  ps.maxParticles = maxCount;
  ps.particles.resize(maxCount);
  for (auto &p : ps.particles)
    p.active = false;
}

static Particle *GetFreeParticle(ParticleSystem &ps) {
  for (auto &p : ps.particles)
    if (!p.active)
      return &p;
  return nullptr;
}

void ParticleSystemUpdate(ParticleSystem &ps, float dt) {
  for (auto &p : ps.particles) {
    if (!p.active)
      continue;
    p.life -= dt;
    if (p.life <= 0) {
      p.active = false;
      continue;
    }

    float ratio = p.life / p.maxLife;
    p.color.a = (unsigned char)(255 * ratio);

    switch (p.type) {
    case ParticleType::RAIN_DROP:
      p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
      break;
    case ParticleType::SHOCKWAVE:
      // Expand radius stored in rotation field
      p.rotation += 12.0f * dt;
      p.color.a = (unsigned char)(200 * ratio);
      break;
    case ParticleType::SMOKE_PUFF:
      p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
      p.size += 0.04f * dt;
      break;
    default:
      p.velocity.y -= 9.8f * dt;
      p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
      break;
    }
  }
}

void ParticleSystemDraw(const ParticleSystem &ps) {
  for (const auto &p : ps.particles) {
    if (!p.active)
      continue;
    switch (p.type) {
    case ParticleType::RAIN_DROP: {
      Vector3 end = Vector3Add(p.position, {0, -0.3f, 0});
      DrawLine3D(p.position, end, p.color);
      break;
    }
    case ParticleType::SHOCKWAVE:
      // Ring of cubes expanding outward
      for (int i = 0; i < 12; i++) {
        float a = (float)i / 12.0f * 6.2832f;
        Vector3 rp = {p.position.x + cosf(a) * p.rotation, p.position.y + 0.1f,
                      p.position.z + sinf(a) * p.rotation};
        DrawCube(rp, p.size, 0.05f, p.size, p.color);
      }
      break;
    case ParticleType::SMOKE_PUFF:
      DrawSphere(p.position, p.size, p.color);
      break;
    case ParticleType::SPARK:
      DrawSphere(p.position, 0.035f, p.color);
      break;
    case ParticleType::MUZZLE_FLASH:
      DrawSphere(p.position, p.size * 1.2f, p.color);
      break;
    default:
      DrawSphere(p.position, p.size, p.color);
      break;
    }
  }
}

void ParticleSpawnMuzzleFlash(ParticleSystem &ps, Vector3 pos, Vector3 dir) {
  for (int i = 0; i < 6; i++) {
    Particle *p = GetFreeParticle(ps);
    if (!p)
      break;
    p->active = true;
    p->type = ParticleType::MUZZLE_FLASH;
    p->position = Vector3Add(pos, Vector3Scale(dir, RandF(0.02f, 0.08f)));
    p->velocity = {RandF(-0.5f, 0.5f), RandF(0, 1.0f), RandF(-0.5f, 0.5f)};
    p->life = p->maxLife = RandF(0.04f, 0.09f);
    p->size = RandF(0.04f, 0.09f);
    p->color = {255, (unsigned char)RandF(180, 255), 50, 255};
  }
}

void ParticleSpawnImpact(ParticleSystem &ps, Vector3 pos, Vector3 normal) {
  for (int i = 0; i < 8; i++) {
    Particle *p = GetFreeParticle(ps);
    if (!p)
      break;
    p->active = true;
    p->type = ParticleType::IMPACT_DUST;
    p->position = pos;
    Vector3 spread = {RandF(-1, 1), RandF(0, 2), RandF(-1, 1)};
    p->velocity = Vector3Add(Vector3Scale(normal, 1.5f), spread);
    p->life = p->maxLife = RandF(0.3f, 0.6f);
    p->size = RandF(0.02f, 0.05f);
    unsigned char g = (unsigned char)RandF(160, 200);
    p->color = {g, g, g, 220};
  }
}

void ParticleSpawnBlood(ParticleSystem &ps, Vector3 pos) {
  for (int i = 0; i < 12; i++) {
    Particle *p = GetFreeParticle(ps);
    if (!p)
      break;
    p->active = true;
    p->type = ParticleType::BLOOD;
    p->position = pos;
    p->velocity = {RandF(-2, 2), RandF(1, 4), RandF(-2, 2)};
    p->life = p->maxLife = RandF(0.4f, 0.8f);
    p->size = RandF(0.025f, 0.06f);
    p->color = {180, 20, 20, 255};
  }
}

void ParticleSpawnExplosion(ParticleSystem &ps, Vector3 pos) {
  // Debris
  for (int i = 0; i < 30; i++) {
    Particle *p = GetFreeParticle(ps);
    if (!p)
      break;
    p->active = true;
    p->type = ParticleType::EXPLOSION_DEBRIS;
    p->position = pos;
    p->velocity = {RandF(-6, 6), RandF(2, 10), RandF(-6, 6)};
    p->life = p->maxLife = RandF(0.6f, 1.2f);
    p->size = RandF(0.04f, 0.12f);
    float r = RandF(0, 1);
    if (r < 0.5f)
      p->color = {255, (unsigned char)RandF(120, 200), 20, 255};
    else
      p->color = {80, 80, 80, 220};
  }
  // Smoke puffs
  for (int i = 0; i < 10; i++) {
    Particle *p = GetFreeParticle(ps);
    if (!p)
      break;
    p->active = true;
    p->type = ParticleType::SMOKE_PUFF;
    p->position = Vector3Add(pos, {RandF(-1, 1), RandF(0, 0.5f), RandF(-1, 1)});
    p->velocity = {RandF(-0.5f, 0.5f), RandF(1, 3), RandF(-0.5f, 0.5f)};
    p->life = p->maxLife = RandF(1.0f, 2.0f);
    p->size = RandF(0.2f, 0.4f);
    unsigned char v = (unsigned char)RandF(60, 90);
    p->color = {v, v, v, 180};
  }
  // Shockwave
  ParticleSpawnShockwave(ps, pos, 6.0f);
}

void ParticleSpawnSparks(ParticleSystem &ps, Vector3 pos, int count) {
  for (int i = 0; i < count; i++) {
    Particle *p = GetFreeParticle(ps);
    if (!p)
      break;
    p->active = true;
    p->type = ParticleType::SPARK;
    p->position = pos;
    p->velocity = {RandF(-3, 3), RandF(1, 5), RandF(-3, 3)};
    p->life = p->maxLife = RandF(0.15f, 0.4f);
    p->size = 0.03f;
    p->color = {255, (unsigned char)RandF(200, 255), 50, 255};
  }
}

void ParticleSpawnRocketTrail(ParticleSystem &ps, Vector3 pos) {
  for (int i = 0; i < 3; i++) {
    Particle *p = GetFreeParticle(ps);
    if (!p)
      break;
    p->active = true;
    p->type = ParticleType::SMOKE_PUFF;
    p->position = Vector3Add(pos, {RandF(-0.05f, 0.05f), RandF(-0.05f, 0.05f),
                                   RandF(-0.05f, 0.05f)});
    p->velocity = {RandF(-0.2f, 0.2f), RandF(0.1f, 0.3f), RandF(-0.2f, 0.2f)};
    p->life = p->maxLife = RandF(0.5f, 1.0f);
    p->size = RandF(0.05f, 0.12f);
    unsigned char v = (unsigned char)RandF(100, 160);
    p->color = {v, v, v, 160};
  }
}

void ParticleSpawnShockwave(ParticleSystem &ps, Vector3 pos, float maxRadius) {
  Particle *p = GetFreeParticle(ps);
  if (!p)
    return;
  p->active = true;
  p->type = ParticleType::SHOCKWAVE;
  p->position = pos;
  p->velocity = {0, 0, 0};
  p->life = p->maxLife = 0.5f;
  p->size = 0.15f;
  p->rotation = 0.0f; // start radius
  p->color = {255, 200, 80, 200};
  (void)maxRadius;
}

void ParticleSpawnRain(ParticleSystem &ps, Vector3 playerPos, float intensity) {
  int count = (int)(intensity * 15);
  for (int i = 0; i < count; i++) {
    Particle *p = GetFreeParticle(ps);
    if (!p)
      break;
    p->active = true;
    p->type = ParticleType::RAIN_DROP;
    p->position = {playerPos.x + RandF(-20, 20), playerPos.y + RandF(4, 10),
                   playerPos.z + RandF(-20, 20)};
    p->velocity = {RandF(-0.3f, 0.3f), -12.0f, RandF(-0.3f, 0.3f)};
    p->life = p->maxLife = RandF(0.4f, 1.0f);
    p->size = 0.01f;
    p->color = {180, 210, 255, 150};
  }
}
