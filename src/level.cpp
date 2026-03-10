#include "level.h"
#include <math.h>
#include <stdlib.h>

// Day/Night sky colors
static Color DawnSky() { return {200, 140, 100, 255}; }
static Color DaySky() { return {100, 160, 220, 255}; }
static Color DuskSky() { return {220, 100, 60, 255}; }
static Color NightSky() { return {15, 18, 40, 255}; }
static Color LerpColor(Color a, Color b, float t) {
  return {(unsigned char)(a.r + (b.r - a.r) * t),
          (unsigned char)(a.g + (b.g - a.g) * t),
          (unsigned char)(a.b + (b.b - a.b) * t), 255};
}

void LevelLoadDefault(LevelData &level) {
  level.walls.clear();
  level.buildings.clear();
  level.coverObjects.clear();
  level.enemySpawns.clear();

  level.playerSpawn = {0, 0, 0};
  level.skyColor = DaySky();
  level.groundColor = {50, 60, 45, 255};
  level.fogColor = {140, 155, 170, 255};

  // Time/Weather init
  level.timeOfDay = TimeOfDay::DAY;
  level.weather = Weather::CLEAR;
  level.timeTimer = 0;
  level.timePeriodLength = 120.0f; // 2 minutes per time period
  level.weatherTimer = 0;
  level.rainIntensity = 0;
  level.fogDensity = 0;
  level.thunderTimer = 0;
  level.lightningFlash = 0;

  // =================== BUILDINGS ===================
  // Central warehouse (2f)
  level.buildings.push_back(
      {{0, 0, 25}, 14, 7, 10, {90, 90, 85, 255}, {70, 70, 65, 255}, 2, true});
  // SW building (1f)
  level.buildings.push_back({{-30, 0, -20},
                             10,
                             5,
                             10,
                             {100, 85, 75, 255},
                             {80, 65, 55, 255},
                             1,
                             false});
  // NE tower (3f, rooftop)
  level.buildings.push_back(
      {{35, 0, 30}, 6, 12, 6, {85, 90, 95, 255}, {65, 70, 75, 255}, 3, true});
  // SE barracks (1f)
  level.buildings.push_back(
      {{30, 0, -25}, 14, 4, 6, {95, 90, 80, 255}, {75, 70, 60, 255}, 1, false});
  // NW apartment (3f, rooftop)
  level.buildings.push_back(
      {{-40, 0, 35}, 12, 9, 8, {95, 90, 90, 255}, {75, 70, 70, 255}, 3, true});
  // Small shop
  level.buildings.push_back({{-25, 0, 40},
                             6,
                             4,
                             5,
                             {110, 100, 85, 255},
                             {90, 80, 65, 255},
                             1,
                             false});
  // Office (2f)
  level.buildings.push_back(
      {{-45, 0, 15}, 10, 7, 8, {85, 85, 95, 255}, {65, 65, 75, 255}, 2, false});
  // Corner store
  level.buildings.push_back({{-20, 0, 45},
                             7,
                             3.5f,
                             6,
                             {105, 95, 80, 255},
                             {85, 75, 60, 255},
                             1,
                             false});
  // Factory (industrial)
  level.buildings.push_back({{40, 0, -35},
                             16,
                             6,
                             10,
                             {80, 80, 85, 255},
                             {60, 60, 65, 255},
                             1,
                             false});
  // Storage A
  level.buildings.push_back(
      {{50, 0, -20}, 8, 4, 6, {90, 85, 80, 255}, {70, 65, 60, 255}, 1, false});
  // Storage B
  level.buildings.push_back(
      {{50, 0, -10}, 8, 4, 6, {85, 80, 75, 255}, {65, 60, 55, 255}, 1, false});
  // Pump station
  level.buildings.push_back(
      {{35, 0, -50}, 6, 5, 6, {75, 80, 85, 255}, {55, 60, 65, 255}, 1, false});
  // South outpost
  level.buildings.push_back(
      {{10, 0, -45}, 8, 5, 8, {95, 85, 80, 255}, {75, 65, 60, 255}, 1, false});
  // West guard post
  level.buildings.push_back({{-50, 0, -10},
                             5,
                             3.5f,
                             5,
                             {90, 90, 85, 255},
                             {70, 70, 65, 255},
                             1,
                             false});
  // East garage
  level.buildings.push_back(
      {{50, 0, 15}, 10, 4, 8, {90, 80, 70, 255}, {70, 60, 50, 255}, 1, false});

  // =================== WALLS ===================
  level.walls.push_back(
      {{-15, 0, 15}, {1, 3, 20}, {110, 105, 100, 255}, false});
  level.walls.push_back({{15, 0, 15}, {1, 3, 20}, {110, 105, 100, 255}, false});
  level.walls.push_back({{0, 0, 35}, {30, 3, 1}, {110, 105, 100, 255}, false});
  // Perimeter
  level.walls.push_back({{-62, 0, 0}, {1, 5, 124}, {70, 70, 70, 255}, false});
  level.walls.push_back({{62, 0, 0}, {1, 5, 124}, {70, 70, 70, 255}, false});
  level.walls.push_back({{0, 0, -62}, {124, 5, 1}, {70, 70, 70, 255}, false});
  level.walls.push_back({{0, 0, 62}, {124, 5, 1}, {70, 70, 70, 255}, false});
  // Scattered cover
  level.walls.push_back({{-10, 0, 5}, {1, 2, 8}, {100, 95, 90, 255}, false});
  level.walls.push_back({{10, 0, -5}, {8, 2, 1}, {100, 95, 90, 255}, false});
  level.walls.push_back({{-20, 0, 15}, {1, 2, 6}, {105, 100, 95, 255}, false});
  level.walls.push_back({{20, 0, 10}, {6, 2, 1}, {105, 100, 95, 255}, false});

  // =================== COVER OBJECTS ===================
  // Sandbags
  for (int i = 0; i < 8; i++) {
    float angle = (float)i / 8.0f * 6.28f;
    level.coverObjects.push_back({{cosf(angle) * 18.0f, 0, sinf(angle) * 18.0f},
                                  {1.5f, 0.8f, 0.6f},
                                  {90, 80, 65, 255},
                                  "sandbag"});
  }
  // Crates (around factory)
  level.coverObjects.push_back(
      {{38, 0, -38}, {1.2f, 1.2f, 1.2f}, {100, 80, 55, 255}, "crate"});
  level.coverObjects.push_back(
      {{42, 0, -38}, {1.2f, 1.2f, 1.2f}, {95, 75, 50, 255}, "crate"});
  level.coverObjects.push_back(
      {{38, 0, -42}, {1.2f, 1.2f, 1.2f}, {105, 85, 60, 255}, "crate"});
  // Barrels
  level.coverObjects.push_back(
      {{-18, 0, -15}, {0.5f, 1.0f, 0.5f}, {60, 80, 60, 255}, "barrel"});
  level.coverObjects.push_back(
      {{18, 0, 20}, {0.5f, 1.0f, 0.5f}, {80, 60, 60, 255}, "barrel"});
  level.coverObjects.push_back(
      {{5, 0, -20}, {0.5f, 1.0f, 0.5f}, {60, 70, 80, 255}, "barrel"});
  // Vehicles
  level.coverObjects.push_back(
      {{-8, 0, -15}, {4.0f, 1.8f, 2.0f}, {50, 55, 50, 255}, "vehicle"});
  level.coverObjects.push_back(
      {{15, 0, 8}, {4.5f, 1.8f, 2.0f}, {55, 50, 45, 255}, "vehicle"});
  level.coverObjects.push_back(
      {{-35, 0, 25}, {3.8f, 1.6f, 1.8f}, {45, 50, 55, 255}, "vehicle"});

  // =================== ENEMY SPAWNS ===================
  std::vector<Vector3> spawns = {
      {-25, 0, -5}, {25, 0, 5},   {-10, 0, -25}, {10, 0, 25},
      {-40, 0, 20}, {40, 0, -20}, {0, 0, -40},   {0, 0, 40},
      {-15, 0, 15}, {15, 0, -15}, {-30, 0, 30},  {30, 0, -30},
      {-50, 0, -5}, {50, 0, 5},   {-5, 0, -50},  {5, 0, 50}};
  level.enemySpawns = spawns;
}

void LevelUpdate(LevelData &level, float dt) {
  // ===== TIME OF DAY =====
  level.timeTimer += dt;
  if (level.timeTimer >= level.timePeriodLength) {
    level.timeTimer = 0;
    // Cycle: DAWN -> DAY -> DUSK -> NIGHT -> DAWN
    switch (level.timeOfDay) {
    case TimeOfDay::DAWN:
      level.timeOfDay = TimeOfDay::DAY;
      break;
    case TimeOfDay::DAY:
      level.timeOfDay = TimeOfDay::DUSK;
      break;
    case TimeOfDay::DUSK:
      level.timeOfDay = TimeOfDay::NIGHT;
      break;
    case TimeOfDay::NIGHT:
      level.timeOfDay = TimeOfDay::DAWN;
      break;
    }
  }

  // Lerp sky color smoothly
  float t = level.timeTimer / level.timePeriodLength;
  Color from, to;
  switch (level.timeOfDay) {
  case TimeOfDay::DAWN:
    from = NightSky();
    to = DawnSky();
    break;
  case TimeOfDay::DAY:
    from = DawnSky();
    to = DaySky();
    break;
  case TimeOfDay::DUSK:
    from = DaySky();
    to = DuskSky();
    break;
  case TimeOfDay::NIGHT:
    from = DuskSky();
    to = NightSky();
    break;
  }
  level.skyColor = LerpColor(from, to, t);

  // Ground color changes at night
  float nightFactor = (level.timeOfDay == TimeOfDay::NIGHT) ? 0.4f
                      : (level.timeOfDay == TimeOfDay::DAWN ||
                         level.timeOfDay == TimeOfDay::DUSK)
                          ? 0.7f
                          : 1.0f;
  level.groundColor = {(unsigned char)(50 * nightFactor),
                       (unsigned char)(60 * nightFactor),
                       (unsigned char)(45 * nightFactor), 255};

  // ===== WEATHER =====
  level.weatherTimer += dt;
  if (level.weatherTimer > 60.0f)
    level.weatherTimer = 0; // refresh cycle

  // Rain & fog intensity
  if (level.weather == Weather::RAIN || level.weather == Weather::STORM) {
    level.rainIntensity = fminf(level.rainIntensity + dt * 0.2f,
                                level.weather == Weather::STORM ? 1.0f : 0.6f);
    level.fogDensity = level.rainIntensity * 0.4f;
  } else if (level.weather == Weather::FOG) {
    level.rainIntensity = 0;
    level.fogDensity = fminf(level.fogDensity + dt * 0.1f, 0.7f);
  } else {
    level.rainIntensity = fmaxf(level.rainIntensity - dt * 0.3f, 0);
    level.fogDensity = fmaxf(level.fogDensity - dt * 0.15f, 0);
  }

  // Lightning in storms
  if (level.weather == Weather::STORM) {
    level.thunderTimer -= dt;
    if (level.thunderTimer <= 0) {
      level.lightningFlash = 0.15f;
      level.thunderTimer = (float)(rand() % 8 + 3);
    }
  }
  if (level.lightningFlash > 0)
    level.lightningFlash -= dt * 2.0f;
}

static void DrawBuilding(const Building &b) {
  float hw = b.width / 2.0f;
  float hd = b.depth / 2.0f;
  float fh = b.height / b.floors;

  for (int f = 0; f < b.floors; f++) {
    float fy = b.position.y + f * fh;
    Vector3 floorCenter = {b.position.x, fy + fh / 2.0f, b.position.z};
    Color wc = {(unsigned char)fmaxf(b.wallColor.r - f * 8, 40),
                (unsigned char)fmaxf(b.wallColor.g - f * 8, 40),
                (unsigned char)fmaxf(b.wallColor.b - f * 8, 40), 255};

    // 4 walls
    DrawCube({floorCenter.x, floorCenter.y, b.position.z + hd}, b.width, fh,
             0.4f, wc);
    DrawCube({floorCenter.x, floorCenter.y, b.position.z - hd}, b.width, fh,
             0.4f, wc);
    DrawCube({b.position.x - hw, floorCenter.y, floorCenter.z}, 0.4f, fh,
             b.depth, wc);
    DrawCube({b.position.x + hw, floorCenter.y, floorCenter.z}, 0.4f, fh,
             b.depth, wc);

    // Windows per floor
    int numWin = (int)(b.width / 2.5f);
    for (int w = 0; w < numWin; w++) {
      float wx = b.position.x - hw + (w + 0.5f) * b.width / numWin;
      Color winColor = {100, 150, 200, 200};
      DrawCube({wx, fy + fh * 0.6f, b.position.z + hd + 0.05f}, 0.8f, 0.7f,
               0.05f, winColor);
      DrawCube({wx, fy + fh * 0.6f, b.position.z - hd - 0.05f}, 0.8f, 0.7f,
               0.05f, winColor);
    }

    // Floor separator line between floors
    if (f > 0) {
      Color sepColor = {(unsigned char)fmaxf(b.wallColor.r - 20, 0),
                        (unsigned char)fmaxf(b.wallColor.g - 20, 0),
                        (unsigned char)fmaxf(b.wallColor.b - 20, 0), 255};
      DrawCube({floorCenter.x, fy, floorCenter.z}, b.width + 0.1f, 0.1f,
               b.depth + 0.1f, sepColor);
    }
  }

  // Roof
  float roofY = b.position.y + b.height;
  DrawCube({b.position.x, roofY + 0.1f, b.position.z}, b.width + 0.2f, 0.2f,
           b.depth + 0.2f, b.roofColor);

  // Rooftop access: railing
  if (b.hasRooftopAccess) {
    Color railing = {60, 60, 65, 255};
    DrawCube({b.position.x, roofY + 0.5f, b.position.z + hd}, b.width, 0.06f,
             0.06f, railing);
    DrawCube({b.position.x, roofY + 0.5f, b.position.z - hd}, b.width, 0.06f,
             0.06f, railing);
    DrawCube({b.position.x + hw, roofY + 0.5f, b.position.z}, 0.06f, 0.06f,
             b.depth, railing);
    DrawCube({b.position.x - hw, roofY + 0.5f, b.position.z}, 0.06f, 0.06f,
             b.depth, railing);
    // Access hatch
    DrawCube({b.position.x - hw * 0.5f, roofY + 0.15f, b.position.z}, 0.8f,
             0.3f, 0.8f, {50, 50, 50, 255});
  }

  // Door
  Color doorColor = {50, 40, 30, 255};
  DrawCube({b.position.x, b.position.y + 1.0f, b.position.z + hd + 0.05f}, 0.9f,
           2.0f, 0.1f, doorColor);
}

static void DrawCoverObject(const CoverObject &c) {
  if (c.type == "sandbag") {
    DrawCylinder({c.position.x, c.position.y + 0.4f, c.position.z},
                 c.size.x * 0.6f, c.size.x * 0.5f, c.size.y, 6, c.color);
    DrawSphere({c.position.x, c.position.y + c.size.y, c.position.z},
               c.size.x * 0.4f, c.color);
  } else if (c.type == "barrel") {
    DrawCylinder({c.position.x, c.position.y, c.position.z}, c.size.x, c.size.x,
                 c.size.y, 10, c.color);
    Color top = {(unsigned char)(c.color.r - 15),
                 (unsigned char)(c.color.g - 15),
                 (unsigned char)(c.color.b - 15), 255};
    DrawCylinder({c.position.x, c.position.y + c.size.y, c.position.z},
                 c.size.x, c.size.x, 0.06f, 10, top);
  } else if (c.type == "crate") {
    DrawCube({c.position.x, c.position.y + c.size.y / 2.0f, c.position.z},
             c.size.x, c.size.y, c.size.z, c.color);
    // Cross lines on crate
    Color line = {(unsigned char)(c.color.r - 25),
                  (unsigned char)(c.color.g - 15),
                  (unsigned char)(c.color.b - 10), 255};
    DrawCube({c.position.x, c.position.y + c.size.y / 2.0f,
              c.position.z + c.size.z / 2.0f + 0.01f},
             c.size.x, 0.04f, 0.04f, line);
    DrawCube({c.position.x, c.position.y + c.size.y / 2.0f,
              c.position.z + c.size.z / 2.0f + 0.01f},
             0.04f, c.size.y, 0.04f, line);
  } else if (c.type == "vehicle") {
    float vx = c.position.x, vz = c.position.z;
    // Body
    DrawCube({vx, c.position.y + 0.9f, vz}, c.size.x, 1.2f, c.size.z, c.color);
    // Cabin
    DrawCube({vx - c.size.x * 0.1f, c.position.y + 1.7f, vz}, c.size.x * 0.6f,
             0.8f, c.size.z * 0.85f, c.color);
    // Windshield
    DrawCube({vx + c.size.x * 0.18f, c.position.y + 1.85f, vz}, 0.05f, 0.55f,
             c.size.z * 0.75f, {100, 150, 200, 180});
    // Wheels
    Color wc = {30, 30, 30, 255};
    for (int w = 0; w < 4; w++) {
      float wx2 =
          vx + (w < 2 ? -c.size.x / 2.0f - 0.05f : c.size.x / 2.0f + 0.05f);
      float wz2 = vz + (w % 2 == 0 ? c.size.z * 0.32f : -c.size.z * 0.32f);
      DrawCylinder({wx2, c.position.y + 0.35f, wz2}, 0.35f, 0.35f, 0.2f, 10,
                   wc);
    }
    // Headlights
    DrawSphere({vx + c.size.x / 2.0f + 0.06f, c.position.y + 0.9f,
                vz + c.size.z * 0.25f},
               0.12f, {255, 255, 180, 220});
    DrawSphere({vx + c.size.x / 2.0f + 0.06f, c.position.y + 0.9f,
                vz - c.size.z * 0.25f},
               0.12f, {255, 255, 180, 220});
  } else {
    DrawCube({c.position.x, c.position.y + c.size.y / 2.0f, c.position.z},
             c.size.x, c.size.y, c.size.z, c.color);
  }
}

void LevelDraw(const LevelData &level) {
  // Lightning flash overlay
  if (level.lightningFlash > 0) {
    Color lc = {255, 255, 255, (unsigned char)(level.lightningFlash * 100)};
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), lc);
  }

  // Ground
  DrawPlane({0, 0, 0}, {124, 124}, level.groundColor);

  // Ground details: roads
  DrawCube({0, 0.02f, 0}, 5.0f, 0.01f, 124.0f,
           {40, 40, 40, 255}); // main road N-S
  DrawCube({0, 0.02f, 0}, 124.0f, 0.01f, 5.0f,
           {40, 40, 40, 255}); // main road E-W
  // Road markings
  for (int i = -12; i <= 12; i++) {
    DrawCube({0, 0.03f, (float)i * 5.0f}, 0.15f, 0.02f, 2.0f,
             {200, 200, 50, 255});
    DrawCube({(float)i * 5.0f, 0.03f, 0}, 2.0f, 0.02f, 0.15f,
             {200, 200, 50, 255});
  }

  // Ground patches (varied terrain)
  for (int i = -3; i <= 3; i++) {
    for (int j = -3; j <= 3; j++) {
      if ((i + j) % 2 == 0)
        continue;
      DrawCube({(float)i * 18.0f, 0.01f, (float)j * 18.0f}, 8, 0.01f, 8,
               {(unsigned char)(42 + (i * j % 5) * 3),
                (unsigned char)(52 + (i * j % 4) * 4), 38, 255});
    }
  }

  // Trees
  static float treeX[] = {-55, -48, 55, 48, -55, 55, -48, 48};
  static float treeZ[] = {55, 48, -55, -48, -48, 48, 55, -55};
  for (int i = 0; i < 8; i++) {
    float tx = treeX[i], tz = treeZ[i];
    // Trunk
    DrawCylinderEx({tx, 0, tz}, {tx, 3.5f, tz}, 0.2f, 0.1f, 6,
                   {70, 50, 30, 255});
    // Canopy (3 spheres)
    DrawSphere({tx, 4.2f, tz}, 1.8f, {40, 85, 35, 255});
    DrawSphere({tx - 0.8f, 3.6f, tz + 0.5f}, 1.3f, {35, 75, 30, 255});
    DrawSphere({tx + 0.7f, 3.7f, tz - 0.6f}, 1.2f, {45, 90, 38, 255});
  }

  // Street lamps
  for (int i = -2; i <= 2; i++) {
    float lx = (float)i * 22.0f;
    // Pole
    DrawCylinderEx({lx, 0, 10}, {lx, 5, 10}, 0.08f, 0.06f, 6,
                   {70, 70, 75, 255});
    DrawCylinderEx({lx, 5, 10}, {lx + 0.5f, 5.5f, 10}, 0.06f, 0.04f, 4,
                   {70, 70, 75, 255});
    // Lamp
    DrawSphere({lx + 0.5f, 5.6f, 10}, 0.22f, {255, 240, 180, 220});
    // South side
    DrawCylinderEx({lx, 0, -10}, {lx, 5, -10}, 0.08f, 0.06f, 6,
                   {70, 70, 75, 255});
    DrawCylinderEx({lx, 5, -10}, {lx + 0.5f, 5.5f, -10}, 0.06f, 0.04f, 4,
                   {70, 70, 75, 255});
    DrawSphere({lx + 0.5f, 5.6f, -10}, 0.22f, {255, 240, 180, 220});
  }

  // Walls
  for (const auto &w : level.walls) {
    DrawCube({w.position.x, w.position.y + w.size.y / 2.0f, w.position.z},
             w.size.x, w.size.y, w.size.z, w.color);
    // Crenellations on tall walls
    if (w.size.y >= 4.0f && w.size.x == 1.0f) {
      int count = (int)(w.size.z / 2.0f);
      for (int i = 0; i < count; i++) {
        DrawCube({w.position.x, w.position.y + w.size.y + 0.25f,
                  w.position.z - w.size.z / 2.0f + i * 2.0f + 1.0f},
                 w.size.x + 0.1f, 0.5f, 0.8f, w.color);
      }
    }
  }

  // Buildings
  for (const auto &b : level.buildings)
    DrawBuilding(b);

  // Cover objects
  for (const auto &c : level.coverObjects)
    DrawCoverObject(c);

  // Underground tunnel entrance (new zone!)
  DrawCube({0, -0.5f, -55}, 6, 1, 8, {50, 50, 55, 255});  // entrance
  DrawCube({0, -1.5f, -50}, 6, 3, 30, {45, 45, 50, 255}); // tunnel
  // Tunnel lights
  for (int t = 0; t < 4; t++) {
    DrawSphere({0, 0, -45.0f + t * 5.0f}, 0.2f, {200, 200, 100, 200});
  }
  // Tunnel exit
  DrawCube({0, -0.5f, -35}, 6, 1, 4, {50, 50, 55, 255});

  // Fog overlay at edges (simple gradient using planes)
  if (level.fogDensity > 0.05f) {
    unsigned char fogA = (unsigned char)(level.fogDensity * 120);
    Color fogC = {level.fogColor.r, level.fogColor.g, level.fogColor.b, fogA};
    // Distance fog suggestion via sky blending handled in main ClearBackground
  }
}

std::vector<BoundingBox> LevelGetAllColliders(const LevelData &level) {
  std::vector<BoundingBox> boxes;
  // Walls
  for (const auto &w : level.walls) {
    boxes.push_back({{w.position.x - w.size.x / 2, w.position.y,
                      w.position.z - w.size.z / 2},
                     {w.position.x + w.size.x / 2, w.position.y + w.size.y,
                      w.position.z + w.size.z / 2}});
  }
  // Buildings (bounding box per building)
  for (const auto &b : level.buildings) {
    boxes.push_back(
        {{b.position.x - b.width / 2, b.position.y, b.position.z - b.depth / 2},
         {b.position.x + b.width / 2, b.position.y + b.height,
          b.position.z + b.depth / 2}});
  }
  // Cover objects
  for (const auto &c : level.coverObjects) {
    boxes.push_back({{c.position.x - c.size.x / 2, c.position.y,
                      c.position.z - c.size.z / 2},
                     {c.position.x + c.size.x / 2, c.position.y + c.size.y,
                      c.position.z + c.size.z / 2}});
  }
  return boxes;
}

Color LevelGetSkyColor(const LevelData &level) {
  // Storm darkens sky more
  if (level.weather == Weather::STORM) {
    return {(unsigned char)(level.skyColor.r * 0.5f),
            (unsigned char)(level.skyColor.g * 0.5f),
            (unsigned char)(level.skyColor.b * 0.55f), 255};
  }
  return level.skyColor;
}
