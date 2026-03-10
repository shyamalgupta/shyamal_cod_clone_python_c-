#pragma once
#include "raylib.h"

enum class KillstreakType {
  NONE = 0,
  UAV,       // 3 kills - reveals enemies on minimap
  AIRSTRIKE, // 5 kills - explosions across map
  JUGGERNAUT // 7 kills - double health + speed
};

struct KillstreakReward {
  KillstreakType type;
  bool earned;
  bool activated;
  float duration;
  float timer;
  int requiredKills;
};

struct KillstreakSystem {
  KillstreakReward rewards[3];
  int currentStreak;
  int highestStreak;

  // Active effects
  bool uavActive;
  float uavTimer;

  bool juggernautActive;
  float juggernautTimer;

  bool airstrikeActive;
  float airstrikeTimer;
  float airstrikeX;
  float airstrikeZ;
  int airstrikePhase;
  float airstrikePhaseTimer;
};

void KillstreakInit(KillstreakSystem &ks);
void KillstreakUpdate(KillstreakSystem &ks, float dt);
void KillstreakOnKill(KillstreakSystem &ks);
void KillstreakOnDeath(KillstreakSystem &ks);
bool KillstreakActivate(KillstreakSystem &ks);
bool KillstreakHasAvailable(const KillstreakSystem &ks);
KillstreakType KillstreakGetNextAvailable(const KillstreakSystem &ks);
