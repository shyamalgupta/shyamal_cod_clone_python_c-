#pragma once
#include "raylib.h"
#include <vector>

enum class WaveState { INTERMISSION, ACTIVE, COMPLETE, GAME_OVER };

struct WaveSystem {
  int currentWave;
  int enemiesThisWave;
  int enemiesAlive;
  int totalEnemiesKilled;
  int score;

  WaveState state;
  float stateTimer;
  float intermissionDuration;

  // Wave scaling
  float enemyHealthMult;
  float enemySpeedMult;
  float enemyAccuracyMult;

  // Rewards
  int grenadesPerWave;
  int ammoPerWave;

  // Display
  float waveAnnouncementTimer;
  float scorePopTimer;
  int lastScorePop;
  char waveMessage[64];
};

void WaveSystemInit(WaveSystem &ws);
void WaveSystemUpdate(WaveSystem &ws, int aliveEnemies, float dt);
int WaveGetEnemyCount(const WaveSystem &ws);
float WaveGetEnemyHealth(const WaveSystem &ws);
float WaveGetEnemySpeed(const WaveSystem &ws);
float WaveGetEnemyAccuracy(const WaveSystem &ws);
void WaveAddKillScore(WaveSystem &ws, bool headshot);
void WaveSystemStartWave(WaveSystem &ws);
bool WaveIsIntermission(const WaveSystem &ws);
