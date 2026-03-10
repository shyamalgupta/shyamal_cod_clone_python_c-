#include "wave_system.h"
#include <math.h>
#include <stdio.h>
#include <string.h>


void WaveSystemInit(WaveSystem &ws) {
  ws.currentWave = 0;
  ws.enemiesThisWave = 0;
  ws.enemiesAlive = 0;
  ws.totalEnemiesKilled = 0;
  ws.score = 0;
  ws.state = WaveState::INTERMISSION;
  ws.stateTimer = 5.0f; // Initial countdown before wave 1
  ws.intermissionDuration = 8.0f;
  ws.enemyHealthMult = 1.0f;
  ws.enemySpeedMult = 1.0f;
  ws.enemyAccuracyMult = 1.0f;
  ws.grenadesPerWave = 2;
  ws.ammoPerWave = 60;
  ws.waveAnnouncementTimer = 0.0f;
  ws.scorePopTimer = 0.0f;
  ws.lastScorePop = 0;
  snprintf(ws.waveMessage, sizeof(ws.waveMessage), "GET READY!");
}

void WaveSystemUpdate(WaveSystem &ws, int aliveEnemies, float dt) {
  ws.enemiesAlive = aliveEnemies;

  if (ws.waveAnnouncementTimer > 0)
    ws.waveAnnouncementTimer -= dt;
  if (ws.scorePopTimer > 0)
    ws.scorePopTimer -= dt;

  switch (ws.state) {
  case WaveState::INTERMISSION:
    ws.stateTimer -= dt;
    if (ws.stateTimer <= 0) {
      WaveSystemStartWave(ws);
    }
    break;

  case WaveState::ACTIVE:
    if (aliveEnemies <= 0 && ws.enemiesThisWave > 0) {
      ws.state = WaveState::COMPLETE;
      ws.stateTimer = 3.0f; // Show "WAVE COMPLETE" for 3 seconds

      // Wave completion bonus
      int waveBonus = 500 + ws.currentWave * 100;
      ws.score += waveBonus;
      ws.lastScorePop = waveBonus;
      ws.scorePopTimer = 2.0f;
      snprintf(ws.waveMessage, sizeof(ws.waveMessage), "WAVE %d COMPLETE! +%d",
               ws.currentWave, waveBonus);
      ws.waveAnnouncementTimer = 3.0f;
    }
    break;

  case WaveState::COMPLETE:
    ws.stateTimer -= dt;
    if (ws.stateTimer <= 0) {
      ws.state = WaveState::INTERMISSION;
      ws.stateTimer = ws.intermissionDuration;
      snprintf(ws.waveMessage, sizeof(ws.waveMessage), "WAVE %d INCOMING...",
               ws.currentWave + 1);
      ws.waveAnnouncementTimer = ws.intermissionDuration;
    }
    break;

  case WaveState::GAME_OVER:
    // Do nothing, wait for restart
    break;
  }
}

void WaveSystemStartWave(WaveSystem &ws) {
  ws.currentWave++;
  ws.state = WaveState::ACTIVE;

  // Calculate enemies: base 6 + 3 per wave
  ws.enemiesThisWave = 6 + (ws.currentWave - 1) * 3;
  if (ws.enemiesThisWave > 30)
    ws.enemiesThisWave = 30; // Cap at 30

  // Scale difficulty
  ws.enemyHealthMult = 1.0f + (ws.currentWave - 1) * 0.1f;
  ws.enemySpeedMult = 1.0f + (ws.currentWave - 1) * 0.05f;
  ws.enemyAccuracyMult = 1.0f + (ws.currentWave - 1) * 0.02f;

  snprintf(ws.waveMessage, sizeof(ws.waveMessage), "WAVE %d", ws.currentWave);
  ws.waveAnnouncementTimer = 3.0f;
}

int WaveGetEnemyCount(const WaveSystem &ws) { return ws.enemiesThisWave; }

float WaveGetEnemyHealth(const WaveSystem &ws) {
  return 100.0f * ws.enemyHealthMult;
}

float WaveGetEnemySpeed(const WaveSystem &ws) {
  return 3.5f * ws.enemySpeedMult;
}

float WaveGetEnemyAccuracy(const WaveSystem &ws) {
  float acc = 0.35f * ws.enemyAccuracyMult;
  return acc > 0.8f ? 0.8f : acc; // Cap accuracy
}

void WaveAddKillScore(WaveSystem &ws, bool headshot) {
  int killScore = headshot ? 200 : 100;
  ws.score += killScore;
  ws.totalEnemiesKilled++;
  ws.lastScorePop = killScore;
  ws.scorePopTimer = 1.0f;
}

bool WaveIsIntermission(const WaveSystem &ws) {
  return ws.state == WaveState::INTERMISSION || ws.state == WaveState::COMPLETE;
}
