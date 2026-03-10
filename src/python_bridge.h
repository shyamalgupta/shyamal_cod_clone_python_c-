#pragma once
#include "weapon.h"
#include "level.h"
#include <string>

bool PythonBridgeInit(const std::string& scriptsPath);
void PythonBridgeShutdown();
bool PythonBridgeLoadWeapons(WeaponStats outStats[3]);
bool PythonBridgeLoadLevel(LevelData& level);
bool PythonBridgeLoadAIParams(float& detectionRange, float& attackRange, float& accuracy, float& reactionTime);
void PythonBridgeOnKill(int killCount);
void PythonBridgeOnDamage(float damage, float healthRemaining);
