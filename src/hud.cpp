#include "hud.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void HUDInit(HUD &hud) {
  hud.killFeedCount = 0;
  hud.hitmarkerTimer = 0.0f;
  hud.damageIndicatorTimer = 0.0f;
  hud.damageDirection = 0.0f;
  hud.notificationTimer = 0.0f;
  memset(hud.killFeed, 0, sizeof(hud.killFeed));
  memset(hud.notification, 0, sizeof(hud.notification));
}

void HUDUpdate(HUD &hud, float dt) {
  if (hud.hitmarkerTimer > 0)
    hud.hitmarkerTimer -= dt;
  if (hud.damageIndicatorTimer > 0)
    hud.damageIndicatorTimer -= dt;
  if (hud.notificationTimer > 0)
    hud.notificationTimer -= dt;

  // Update kill feed timers
  for (int i = 0; i < hud.killFeedCount; i++) {
    hud.killFeed[i].timer -= dt;
  }
  // Remove expired entries
  while (hud.killFeedCount > 0 && hud.killFeed[0].timer <= 0) {
    for (int i = 0; i < hud.killFeedCount - 1; i++) {
      hud.killFeed[i] = hud.killFeed[i + 1];
    }
    hud.killFeedCount--;
  }
}

static void DrawCrosshair(int screenW, int screenH, bool isAiming,
                          bool isCrouching) {
  int cx = screenW / 2;
  int cy = screenH / 2;
  int gap = isAiming ? 3 : (isCrouching ? 4 : 6);
  int len = isAiming ? 8 : (isCrouching ? 10 : 12);
  int thick = 2;
  Color crossColor = {200, 255, 200, 200};
  Color shadowColor = {0, 0, 0, 120};

  // Shadow
  DrawRectangle(cx - len - gap + 1, cy - thick / 2 + 1, len, thick,
                shadowColor);
  DrawRectangle(cx + gap + 1, cy - thick / 2 + 1, len, thick, shadowColor);
  DrawRectangle(cx - thick / 2 + 1, cy - len - gap + 1, thick, len,
                shadowColor);
  DrawRectangle(cx - thick / 2 + 1, cy + gap + 1, thick, len, shadowColor);

  // Crosshair lines
  DrawRectangle(cx - len - gap, cy - thick / 2, len, thick, crossColor);
  DrawRectangle(cx + gap, cy - thick / 2, len, thick, crossColor);
  DrawRectangle(cx - thick / 2, cy - len - gap, thick, len, crossColor);
  DrawRectangle(cx - thick / 2, cy + gap, thick, len, crossColor);

  // Center dot
  DrawRectangle(cx - 1, cy - 1, 2, 2, WHITE);
}

static void DrawHealthBar(int screenW, int screenH, float health,
                          float maxHealth, float stamina, float maxStamina,
                          bool isCrouching) {
  int barW = 200;
  int barH = 18;
  int x = 30;
  int y = screenH - 50;
  float pct = health / maxHealth;

  // Background
  DrawRectangle(x - 2, y - 2, barW + 4, barH + 4, {0, 0, 0, 160});
  DrawRectangle(x, y, barW, barH, {40, 40, 40, 200});

  // Health fill
  Color healthColor;
  if (pct > 0.6f)
    healthColor = {50, 200, 80, 255};
  else if (pct > 0.3f)
    healthColor = {220, 180, 40, 255};
  else
    healthColor = {220, 50, 50, 255};

  DrawRectangle(x, y, (int)(barW * pct), barH, healthColor);

  // Text
  char healthText[32];
  snprintf(healthText, sizeof(healthText), "HP %.0f", health);
  DrawText(healthText, x + 5, y + 2, 14, WHITE);

  // Health icon
  DrawText("+", x - 20, y, 20, healthColor);

  // Stamina bar (smaller, below health)
  int staminaBarH = 6;
  int staminaY = y + barH + 4;
  float staminaPct = stamina / maxStamina;
  DrawRectangle(x, staminaY, barW, staminaBarH, {30, 30, 30, 150});
  Color staminaColor = staminaPct > 0.3f ? (Color){80, 180, 255, 220}
                                         : (Color){255, 100, 60, 220};
  DrawRectangle(x, staminaY, (int)(barW * staminaPct), staminaBarH,
                staminaColor);

  // Crouch indicator
  if (isCrouching) {
    DrawText("CROUCH", x + barW + 10, y + 2, 12, {180, 180, 255, 200});
  }
}

static void DrawAmmoCounter(int screenW, int screenH, const Weapon &w) {
  int x = screenW - 200;
  int y = screenH - 55;

  // Background
  DrawRectangle(x - 10, y - 5, 190, 50, {0, 0, 0, 140});

  // Current ammo (big)
  char ammoText[32];
  snprintf(ammoText, sizeof(ammoText), "%d", w.currentAmmo);
  DrawText(ammoText, x, y, 30, WHITE);

  // Reserve ammo
  char reserveText[32];
  snprintf(reserveText, sizeof(reserveText), "/ %d", w.reserveAmmo);
  DrawText(reserveText, x + 60, y + 10, 18, {180, 180, 180, 255});

  // Weapon name
  DrawText(w.stats.name.c_str(), x, y + 35, 12, {150, 200, 150, 255});

  // Reload indicator
  if (w.isReloading) {
    float rpct = 1.0f - (w.reloadTimer / w.stats.reloadTime);
    DrawRectangle(x, y - 12, 150, 6, {40, 40, 40, 200});
    DrawRectangle(x, y - 12, (int)(150 * rpct), 6, {100, 200, 255, 255});
    DrawText("RELOADING", x + 50, y - 14, 10, {100, 200, 255, 255});
  }
}

static void DrawMinimap(int screenW, int screenH, const Player &player,
                        const EnemyManager &em, const LevelData &level,
                        bool uavActive) {
  int mapSize = 170;
  int mapX = screenW - mapSize - 15;
  int mapY = 15;
  float scale = mapSize / 124.0f;

  // Background
  DrawRectangle(mapX - 2, mapY - 2, mapSize + 4, mapSize + 4, {0, 0, 0, 180});
  DrawRectangle(mapX, mapY, mapSize, mapSize, {20, 30, 20, 200});

  // Center of minimap
  int cx = mapX + mapSize / 2;
  int cy = mapY + mapSize / 2;

  // Draw buildings
  for (const auto &b : level.buildings) {
    int bx = cx + (int)(b.position.x * scale);
    int by = cy + (int)(-b.position.z * scale);
    int bw = (int)(b.width * scale);
    int bh = (int)(b.depth * scale);
    DrawRectangle(bx - bw / 2, by - bh / 2, bw, bh, {80, 80, 70, 180});
  }

  // Draw walls
  for (const auto &w : level.walls) {
    int wx = cx + (int)(w.position.x * scale);
    int wy = cy + (int)(-w.position.z * scale);
    int ww = (int)(fmaxf(w.size.x * scale, 1));
    int wh = (int)(fmaxf(w.size.z * scale, 1));
    DrawRectangle(wx - ww / 2, wy - wh / 2, ww, wh, {100, 100, 90, 180});
  }

  // Draw cover objects on minimap (vehicles, crates, etc.)
  for (const auto &v : level.coverObjects) {
    if (v.type != "vehicle")
      continue;
    int vx = cx + (int)(v.position.x * scale);
    int vy = cy + (int)(-v.position.z * scale);
    DrawRectangle(vx - 2, vy - 3, 4, 6, {70, 70, 60, 180});
  }

  // Draw enemies (red dots — all visible if UAV active)
  for (const auto &e : em.enemies) {
    if (!e.active || e.state == EnemyState::DEAD)
      continue;
    int ex = cx + (int)(e.position.x * scale);
    int ey = cy + (int)(-e.position.z * scale);
    if (ex >= mapX && ex <= mapX + mapSize && ey >= mapY &&
        ey <= mapY + mapSize) {
      if (uavActive) {
        // Pulsing effect for UAV
        float pulse = (sinf((float)GetTime() * 6.0f) + 1.0f) * 0.5f;
        int dotSize = 3 + (int)(pulse * 2);
        DrawCircle(ex, ey, (float)dotSize, {255, 60, 60, 255});
      } else {
        DrawCircle(ex, ey, 3, {255, 60, 60, 220});
      }
    }
  }

  // Draw player (green triangle)
  int px = cx + (int)(player.position.x * scale);
  int py = cy + (int)(-player.position.z * scale);
  DrawCircle(px, py, 4, {60, 255, 60, 255});

  // Player facing direction
  float dirX = sinf(player.yaw) * 8;
  float dirY = -cosf(player.yaw) * 8;
  DrawLine(px, py, px + (int)dirX, py + (int)dirY, {60, 255, 60, 255});

  // Border
  Color borderColor =
      uavActive ? (Color){100, 200, 255, 200} : (Color){100, 200, 100, 150};
  DrawRectangleLines(mapX - 2, mapY - 2, mapSize + 4, mapSize + 4, borderColor);

  // Label
  const char *mapLabel = uavActive ? "UAV ACTIVE" : "MAP";
  Color labelColor =
      uavActive ? (Color){100, 200, 255, 220} : (Color){100, 200, 100, 120};
  DrawText(mapLabel, mapX + 2, mapY + 2, 10, labelColor);
}

static void DrawKillFeed(int screenW, const HUD &hud) {
  int x = screenW - 350;
  int y = 200;
  for (int i = 0; i < hud.killFeedCount; i++) {
    float alpha = hud.killFeed[i].timer / 4.0f;
    if (alpha > 1.0f)
      alpha = 1.0f;
    Color c = hud.killFeed[i].color;
    c.a = (unsigned char)(c.a * alpha);
    DrawText(hud.killFeed[i].message, x, y + i * 22, 14, c);
  }
}

static void DrawHitmarker(int screenW, int screenH, float timer) {
  if (timer <= 0)
    return;
  int cx = screenW / 2;
  int cy = screenH / 2;
  int size = 10;
  float alpha = timer / 0.2f;
  Color c = {255, 255, 255, (unsigned char)(255 * alpha)};

  DrawLine(cx - size, cy - size, cx - size / 2, cy - size / 2, c);
  DrawLine(cx + size, cy - size, cx + size / 2, cy - size / 2, c);
  DrawLine(cx - size, cy + size, cx - size / 2, cy + size / 2, c);
  DrawLine(cx + size, cy + size, cx + size / 2, cy + size / 2, c);
}

static void DrawWeaponSelector(int screenW, int screenH,
                               const WeaponSystem &ws) {
  int y = screenH - 90;
  int boxW = 65;
  int boxH = 25;
  int gap = 5;
  int startX = screenW / 2 - (3 * boxW + 2 * gap) / 2;

  const char *names[] = {"M4A1", "SPAS", "BARR"};
  const char *keys[] = {"1", "2", "3"};

  for (int i = 0; i < 3; i++) {
    int x = startX + i * (boxW + gap);
    bool active = (i == ws.currentWeapon);
    Color bg = active ? Color{60, 120, 60, 200} : Color{40, 40, 40, 150};
    Color border = active ? Color{100, 220, 100, 255} : Color{80, 80, 80, 150};

    DrawRectangle(x, y, boxW, boxH, bg);
    DrawRectangleLines(x, y, boxW, boxH, border);
    DrawText(keys[i], x + 3, y + 2, 10, {180, 180, 180, 200});
    DrawText(names[i], x + 15, y + 7, 12,
             active ? WHITE : Color{150, 150, 150, 200});
  }
}

static void DrawWaveInfo(int screenW, int screenH, const WaveSystem &ws) {
  // Wave number (top left area, below kills)
  char waveText[64];
  snprintf(waveText, sizeof(waveText), "WAVE %d", ws.currentWave);
  DrawText(waveText, 30, 70, 16, {200, 200, 255, 220});

  // Enemies remaining
  if (ws.state == WaveState::ACTIVE) {
    char enemyText[64];
    snprintf(enemyText, sizeof(enemyText), "ENEMIES: %d", ws.enemiesAlive);
    DrawText(enemyText, 30, 90, 14, {255, 180, 180, 200});
  }

  // Score
  char scoreText[64];
  snprintf(scoreText, sizeof(scoreText), "SCORE: %d", ws.score);
  DrawText(scoreText, 30, 110, 14, {255, 220, 100, 220});

  // Wave announcement (centered)
  if (ws.waveAnnouncementTimer > 0) {
    float alpha = fminf(ws.waveAnnouncementTimer / 0.5f, 1.0f);
    int fontSize = 40;
    int textW = MeasureText(ws.waveMessage, fontSize);
    Color msgColor = {255, 255, 255, (unsigned char)(255 * alpha)};
    Color shadowColor = {0, 0, 0, (unsigned char)(180 * alpha)};

    DrawText(ws.waveMessage, screenW / 2 - textW / 2 + 2, screenH / 3 + 2,
             fontSize, shadowColor);
    DrawText(ws.waveMessage, screenW / 2 - textW / 2, screenH / 3, fontSize,
             msgColor);
  }

  // Score pop
  if (ws.scorePopTimer > 0) {
    float alpha = fminf(ws.scorePopTimer / 0.3f, 1.0f);
    char popText[32];
    snprintf(popText, sizeof(popText), "+%d", ws.lastScorePop);
    float yOffset = (1.0f - ws.scorePopTimer) * 30;
    DrawText(popText, screenW / 2 + 40, screenH / 3 + 40 - (int)yOffset, 24,
             {255, 220, 100, (unsigned char)(255 * alpha)});
  }

  // Intermission countdown
  if (ws.state == WaveState::INTERMISSION) {
    char countText[32];
    snprintf(countText, sizeof(countText), "%.0f", ws.stateTimer);
    int tw = MeasureText(countText, 50);
    DrawText(countText, screenW / 2 - tw / 2, screenH / 3 + 50, 50,
             {255, 255, 255, 180});
  }
}

static void DrawKillstreakHUD(int screenW, int screenH,
                              const KillstreakSystem &ks) {
  int x = screenW - 180;
  int y = screenH - 140;

  const char *names[] = {"UAV [3]", "AIRSTRIKE [5]", "JUGGERNAUT [7]"};
  Color colors[] = {
      {100, 200, 255, 255}, // UAV blue
      {255, 150, 50, 255},  // Airstrike orange
      {255, 80, 80, 255}    // Juggernaut red
  };

  for (int i = 0; i < 3; i++) {
    bool earned = ks.rewards[i].earned && !ks.rewards[i].activated;
    bool activated = ks.rewards[i].activated;
    unsigned char alpha = earned ? 255 : (activated ? 80 : 120);

    Color c = colors[i];
    c.a = alpha;

    if (earned) {
      // Flashing available indicator
      float pulse = (sinf((float)GetTime() * 4.0f) + 1.0f) * 0.5f;
      DrawRectangle(x - 3, y + i * 18 - 2, 175, 16,
                    {c.r, c.g, c.b, (unsigned char)(40 * pulse)});
    }

    DrawText(names[i], x, y + i * 18, 12, c);
    if (activated)
      DrawText("x", x - 12, y + i * 18, 12, {100, 100, 100, 150});
    else if (earned)
      DrawText(">", x - 12, y + i * 18, 12, c);
  }

  // Streak counter
  char streakText[32];
  snprintf(streakText, sizeof(streakText), "STREAK: %d", ks.currentStreak);
  DrawText(streakText, x, y - 16, 11, {200, 200, 200, 180});

  // Active effect indicators
  if (ks.uavActive) {
    char uavText[32];
    snprintf(uavText, sizeof(uavText), "UAV %.0fs", ks.uavTimer);
    DrawText(uavText, screenW / 2 - 30, 60, 14, {100, 200, 255, 220});
  }
  if (ks.juggernautActive) {
    char jugText[32];
    snprintf(jugText, sizeof(jugText), "JUGGERNAUT %.0fs", ks.juggernautTimer);
    DrawText(jugText, screenW / 2 - 60, 80, 14, {255, 80, 80, 220});
  }

  // "Press 4 to activate" hint
  if (KillstreakHasAvailable(ks)) {
    DrawText("[4] ACTIVATE KILLSTREAK", screenW / 2 - 80, screenH - 120, 14,
             {255, 255, 100, 200});
  }
}

static void DrawGrenadeCounter(int screenW, int screenH, int grenadeCount) {
  int x = 30;
  int y = screenH - 80;
  DrawRectangle(x - 2, y - 2, 80, 22, {0, 0, 0, 120});

  char grenadeText[32];
  snprintf(grenadeText, sizeof(grenadeText), "GREN: %d", grenadeCount);
  Color gColor = grenadeCount > 0 ? (Color){180, 220, 130, 230}
                                  : (Color){120, 120, 120, 150};
  DrawText(grenadeText, x + 2, y + 2, 14, gColor);

  // Key hint
  DrawText("[G]", x + 60, y + 4, 10, {150, 150, 150, 150});
}

void HUDDraw(const HUD &hud, const Player &player, const WeaponSystem &ws,
             const EnemyManager &em, const LevelData &level,
             const WaveSystem &waves, const KillstreakSystem &ks,
             int grenadeCount) {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  const Weapon &w = WeaponGetCurrentConst(ws);

  // Crosshair
  DrawCrosshair(screenW, screenH, false, player.isCrouching);

  // Hitmarker
  DrawHitmarker(screenW, screenH, hud.hitmarkerTimer);

  // Health bar (with stamina)
  DrawHealthBar(screenW, screenH, player.health, player.maxHealth,
                player.stamina, player.maxStamina, player.isCrouching);

  // Grenade counter
  DrawGrenadeCounter(screenW, screenH, grenadeCount);

  // Ammo counter
  DrawAmmoCounter(screenW, screenH, w);

  // Weapon selector
  DrawWeaponSelector(screenW, screenH, ws);

  // Minimap
  DrawMinimap(screenW, screenH, player, em, level, ks.uavActive);

  // Kill counter
  char killText[64];
  snprintf(killText, sizeof(killText), "KILLS: %d  DEATHS: %d", player.kills,
           player.deaths);
  DrawText(killText, 30, 20, 18, {200, 220, 200, 220});

  // Wave info
  DrawWaveInfo(screenW, screenH, waves);

  // Killstreak HUD
  DrawKillstreakHUD(screenW, screenH, ks);

  // Kill feed
  DrawKillFeed(screenW, hud);

  // Notification popup
  if (hud.notificationTimer > 0) {
    float alpha = fminf(hud.notificationTimer / 0.5f, 1.0f);
    int tw = MeasureText(hud.notification, 18);
    DrawRectangle(screenW / 2 - tw / 2 - 10, screenH / 2 + 60, tw + 20, 28,
                  {0, 0, 0, (unsigned char)(150 * alpha)});
    DrawText(hud.notification, screenW / 2 - tw / 2, screenH / 2 + 65, 18,
             {hud.notificationColor.r, hud.notificationColor.g,
              hud.notificationColor.b, (unsigned char)(255 * alpha)});
  }

  // Damage vignette + low health effects
  if (player.damageFlashTimer > 0) {
    float alpha = player.damageFlashTimer / 0.3f;
    Color damageColor = {200, 0, 0, (unsigned char)(100 * alpha)};
    DrawRectangle(0, 0, screenW, screenH, damageColor);
    DrawRectangle(0, 0, 40, screenH, {200, 0, 0, (unsigned char)(60 * alpha)});
    DrawRectangle(screenW - 40, 0, 40, screenH,
                  {200, 0, 0, (unsigned char)(60 * alpha)});
  }

  // Low health warning (pulsing red edges)
  if (player.health > 0 && player.health < 30.0f) {
    float pulse = (sinf((float)GetTime() * 6.0f) + 1.0f) * 0.5f;
    unsigned char edgeAlpha = (unsigned char)(80 * pulse);
    DrawRectangle(0, 0, 15, screenH, {200, 0, 0, edgeAlpha});
    DrawRectangle(screenW - 15, 0, 15, screenH, {200, 0, 0, edgeAlpha});
    DrawRectangle(0, 0, screenW, 10, {200, 0, 0, edgeAlpha});
    DrawRectangle(0, screenH - 10, screenW, 10, {200, 0, 0, edgeAlpha});
  }

  // FPS counter
  char fpsText[32];
  snprintf(fpsText, sizeof(fpsText), "%d FPS", GetFPS());
  DrawText(fpsText, 30, 45, 14, {150, 150, 150, 180});

  // Death screen
  if (!PlayerIsAlive(player)) {
    DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 180});

    const char *deathMsg = "YOU DIED";
    int textW = MeasureText(deathMsg, 50);
    DrawText(deathMsg, screenW / 2 - textW / 2, screenH / 2 - 80, 50,
             {200, 30, 30, 255});

    // Stats
    char statsText[128];
    snprintf(statsText, sizeof(statsText), "Kills: %d   Wave: %d   Score: %d",
             player.kills, waves.currentWave, waves.score);
    int stW = MeasureText(statsText, 20);
    DrawText(statsText, screenW / 2 - stW / 2, screenH / 2 - 10, 20,
             {200, 200, 200, 220});

    // Best streak
    char streakText[64];
    snprintf(streakText, sizeof(streakText), "Best Streak: %d",
             ks.highestStreak);
    int srW = MeasureText(streakText, 16);
    DrawText(streakText, screenW / 2 - srW / 2, screenH / 2 + 20, 16,
             {180, 180, 180, 200});

    const char *respawnMsg = "Press ENTER to restart";
    int respW = MeasureText(respawnMsg, 20);
    DrawText(respawnMsg, screenW / 2 - respW / 2, screenH / 2 + 60, 20,
             {200, 200, 200, 200});
  }
}

void HUDAddKillFeed(HUD &hud, const char *message, Color color) {
  if (hud.killFeedCount >= 5) {
    for (int i = 0; i < 4; i++) {
      hud.killFeed[i] = hud.killFeed[i + 1];
    }
    hud.killFeedCount = 4;
  }
  strncpy(hud.killFeed[hud.killFeedCount].message, message, 127);
  hud.killFeed[hud.killFeedCount].timer = 4.0f;
  hud.killFeed[hud.killFeedCount].color = color;
  hud.killFeedCount++;
}

void HUDTriggerHitmarker(HUD &hud) { hud.hitmarkerTimer = 0.2f; }

void HUDTriggerDamageIndicator(HUD &hud, float direction) {
  hud.damageIndicatorTimer = 0.5f;
  hud.damageDirection = direction;
}

void HUDShowNotification(HUD &hud, const char *message, Color color) {
  strncpy(hud.notification, message, 127);
  hud.notificationColor = color;
  hud.notificationTimer = 3.0f;
}
