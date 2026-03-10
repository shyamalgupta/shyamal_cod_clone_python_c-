#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>

struct Wall {
  Vector3 position;
  Vector3 size;
  Color color;
  bool isRoof;
};

struct Building {
  Vector3 position;
  float width, height, depth;
  Color wallColor;
  Color roofColor;
  int floors;
  bool hasRooftopAccess; // new!
};

struct CoverObject {
  Vector3 position;
  Vector3 size;
  Color color;
  std::string type; // "crate", "barrel", "sandbag", "vehicle"
};

enum class TimeOfDay { DAWN, DAY, DUSK, NIGHT };
enum class Weather { CLEAR, RAIN, FOG, STORM };
enum class LevelType { DEFAULT, URBAN_CENTER, OUTPOST };

struct LevelData {
  std::vector<Wall> walls;
  std::vector<Building> buildings;
  std::vector<CoverObject> coverObjects;
  std::vector<Vector3> enemySpawns;
  Vector3 playerSpawn;

  // Type
  LevelType levelType;

  // Sky / atmosphere
  Color skyColor;
  Color groundColor;
  Color fogColor;

  // Time & weather (new!)
  TimeOfDay timeOfDay;
  Weather weather;
  float timeTimer;        // seconds elapsed in current time period
  float timePeriodLength; // how long each period lasts (seconds)
  float weatherTimer;
  float rainIntensity; // 0-1
  float fogDensity;    // 0-1

  // Thunder
  float thunderTimer;
  float lightningFlash;
};

void LevelLoadRandom(LevelData &level);
void LevelDraw(const LevelData &level);
void LevelUpdate(LevelData &level, float dt);
std::vector<BoundingBox> LevelGetAllColliders(const LevelData &level);
Color LevelGetSkyColor(const LevelData &level);
