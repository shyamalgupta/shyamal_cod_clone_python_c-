#pragma once
#include "enemy.h"
#include "killstreak.h"
#include "level.h"
#include "player.h"
#include "raylib.h"
#include "raymath.h"
#include "wave_system.h"
#include "weapon.h"

struct KillFeedEntry {
  char message[128];
  float timer;
  Color color;
};

struct FloatingText {
  char text[64];
  float x, y;
  float timer;
  Color color;
};

struct HUD {
  KillFeedEntry killFeed[5];
  int killFeedCount;
  float hitmarkerTimer;
  float damageIndicatorTimer;
  float damageDirection;
  float notificationTimer;
  char notification[128];
  Color notificationColor;
  // Floating XP texts
  FloatingText floatingTexts[16];
  int floatingCount;
};

void HUDInit(HUD &hud);
void HUDUpdate(HUD &hud, float dt);
void HUDDraw(const HUD &hud, const Player &player, const WeaponSystem &ws,
             const EnemyManager &em, const LevelData &level,
             const WaveSystem &waves, const KillstreakSystem &ks,
             int grenadeCount);
void HUDAddKillFeed(HUD &hud, const char *message, Color color);
void HUDTriggerHitmarker(HUD &hud);
void HUDTriggerDamageIndicator(HUD &hud, float direction);
void HUDShowNotification(HUD &hud, const char *message, Color color);
void HUDAddFloatingText(HUD &hud, const char *text, Color color);
