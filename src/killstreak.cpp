#include "killstreak.h"
#include <math.h>
#include <stdlib.h>


void KillstreakInit(KillstreakSystem &ks) {
  // UAV - 3 kills
  ks.rewards[0].type = KillstreakType::UAV;
  ks.rewards[0].earned = false;
  ks.rewards[0].activated = false;
  ks.rewards[0].duration = 15.0f;
  ks.rewards[0].timer = 0.0f;
  ks.rewards[0].requiredKills = 3;

  // Airstrike - 5 kills
  ks.rewards[1].type = KillstreakType::AIRSTRIKE;
  ks.rewards[1].earned = false;
  ks.rewards[1].activated = false;
  ks.rewards[1].duration = 5.0f;
  ks.rewards[1].timer = 0.0f;
  ks.rewards[1].requiredKills = 5;

  // Juggernaut - 7 kills
  ks.rewards[2].type = KillstreakType::JUGGERNAUT;
  ks.rewards[2].earned = false;
  ks.rewards[2].activated = false;
  ks.rewards[2].duration = 20.0f;
  ks.rewards[2].timer = 0.0f;
  ks.rewards[2].requiredKills = 7;

  ks.currentStreak = 0;
  ks.highestStreak = 0;

  ks.uavActive = false;
  ks.uavTimer = 0.0f;
  ks.juggernautActive = false;
  ks.juggernautTimer = 0.0f;
  ks.airstrikeActive = false;
  ks.airstrikeTimer = 0.0f;
  ks.airstrikeX = 0.0f;
  ks.airstrikeZ = 0.0f;
  ks.airstrikePhase = 0;
  ks.airstrikePhaseTimer = 0.0f;
}

void KillstreakUpdate(KillstreakSystem &ks, float dt) {
  // UAV countdown
  if (ks.uavActive) {
    ks.uavTimer -= dt;
    if (ks.uavTimer <= 0) {
      ks.uavActive = false;
    }
  }

  // Juggernaut countdown
  if (ks.juggernautActive) {
    ks.juggernautTimer -= dt;
    if (ks.juggernautTimer <= 0) {
      ks.juggernautActive = false;
    }
  }

  // Airstrike animation
  if (ks.airstrikeActive) {
    ks.airstrikeTimer -= dt;
    ks.airstrikePhaseTimer -= dt;
    if (ks.airstrikeTimer <= 0) {
      ks.airstrikeActive = false;
    }
  }
}

void KillstreakOnKill(KillstreakSystem &ks) {
  ks.currentStreak++;
  if (ks.currentStreak > ks.highestStreak) {
    ks.highestStreak = ks.currentStreak;
  }

  // Check each reward threshold
  for (int i = 0; i < 3; i++) {
    if (ks.currentStreak >= ks.rewards[i].requiredKills &&
        !ks.rewards[i].earned && !ks.rewards[i].activated) {
      ks.rewards[i].earned = true;
    }
  }
}

void KillstreakOnDeath(KillstreakSystem &ks) {
  ks.currentStreak = 0;
  // Reset unactivated rewards
  for (int i = 0; i < 3; i++) {
    if (!ks.rewards[i].activated) {
      ks.rewards[i].earned = false;
    }
  }
}

bool KillstreakActivate(KillstreakSystem &ks) {
  // Find the next available (earned but not activated) killstreak
  for (int i = 0; i < 3; i++) {
    if (ks.rewards[i].earned && !ks.rewards[i].activated) {
      ks.rewards[i].activated = true;

      switch (ks.rewards[i].type) {
      case KillstreakType::UAV:
        ks.uavActive = true;
        ks.uavTimer = ks.rewards[i].duration;
        break;
      case KillstreakType::AIRSTRIKE:
        ks.airstrikeActive = true;
        ks.airstrikeTimer = ks.rewards[i].duration;
        ks.airstrikePhase = 0;
        ks.airstrikePhaseTimer = 0.5f;
        // Random strike position around center of map
        ks.airstrikeX = (float)(rand() % 60 - 30);
        ks.airstrikeZ = (float)(rand() % 60 - 30);
        break;
      case KillstreakType::JUGGERNAUT:
        ks.juggernautActive = true;
        ks.juggernautTimer = ks.rewards[i].duration;
        break;
      default:
        break;
      }
      return true;
    }
  }
  return false;
}

bool KillstreakHasAvailable(const KillstreakSystem &ks) {
  for (int i = 0; i < 3; i++) {
    if (ks.rewards[i].earned && !ks.rewards[i].activated)
      return true;
  }
  return false;
}

KillstreakType KillstreakGetNextAvailable(const KillstreakSystem &ks) {
  for (int i = 0; i < 3; i++) {
    if (ks.rewards[i].earned && !ks.rewards[i].activated) {
      return ks.rewards[i].type;
    }
  }
  return KillstreakType::NONE;
}
