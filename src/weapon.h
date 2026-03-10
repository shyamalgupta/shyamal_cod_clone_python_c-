#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>

enum class WeaponType {
  ASSAULT_RIFLE = 0, // M4A1
  SHOTGUN,           // SPAS-12
  SNIPER,            // Barrett .50
  LMG,               // M249 SAW
  RPG,               // RPG-7
  KNIFE,             // Combat knife
  AXE,               // Battle axe melee
  PISTOL,            // Pistol
  COUNT
};

struct WeaponAttachment {
  bool hasSilencer;    // reduces fire sound, no flash, slight dmg drop
  bool hasRedDot;      // improves accuracy by 15%
  bool hasExtendedMag; // +50% mag capacity
  bool hasForegrip;    // reduces recoil by 30%
};

struct WeaponStats {
  std::string name;
  float damage;
  float fireRate; // shots per second
  int magSize;
  float reloadTime;
  float accuracy; // 0-1, 1 = perfect
  float recoilAmount;
  float range;
  int pelletsPerShot; // >1 for shotgun
  float headshotMult;
  bool isAutomatic;
  bool isExplosive;      // RPG
  float projectileSpeed; // RPG rockets
  float blastRadius;     // for RPG
};

struct Weapon {
  WeaponType type;
  WeaponStats stats;
  int currentAmmo;
  int reserveAmmo;
  float fireCooldown;
  float reloadTimer;
  bool isReloading;
  float recoilTimer;
  float currentRecoil;
  WeaponAttachment attachments;
};

struct WeaponSystem {
  Weapon
      weapons[(int)WeaponType::COUNT]; // includes axe, knife handled separately
  int currentWeapon;
  bool justFired;
  Vector3 muzzleFlashPos;
  // Knife
  bool knifeAttacking;
  float knifeTimer;
  float knifeCooldown;
};

void WeaponSystemInit(WeaponSystem &ws);
void WeaponSystemUpdate(WeaponSystem &ws, float dt);
bool WeaponFire(WeaponSystem &ws);
void WeaponReload(WeaponSystem &ws);
void WeaponSwitch(WeaponSystem &ws, int index);
Weapon &WeaponGetCurrent(WeaponSystem &ws);
const Weapon &WeaponGetCurrentConst(const WeaponSystem &ws);
bool WeaponKnifeAttack(WeaponSystem &ws);
int WeaponGetEffectiveMagSize(const Weapon &w);
float WeaponGetEffectiveAccuracy(const Weapon &w);
float WeaponGetEffectiveRecoil(const Weapon &w);
