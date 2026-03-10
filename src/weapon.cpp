#include "weapon.h"
#include <math.h>
#include <string.h>

static WeaponStats GetDefaultStats(WeaponType t) {
  WeaponStats s = {};
  switch (t) {
  case WeaponType::ASSAULT_RIFLE:
    s.name = "M4A1";
    s.damage = 28;
    s.fireRate = 10;
    s.magSize = 30;
    s.reloadTime = 2.0f;
    s.accuracy = 0.85f;
    s.recoilAmount = 0.04f;
    s.range = 80;
    s.pelletsPerShot = 1;
    s.headshotMult = 2.5f;
    s.isAutomatic = true;
    s.isExplosive = false;
    break;
  case WeaponType::SHOTGUN:
    s.name = "SPAS-12";
    s.damage = 18;
    s.fireRate = 1.2f;
    s.magSize = 8;
    s.reloadTime = 2.5f;
    s.accuracy = 0.6f;
    s.recoilAmount = 0.1f;
    s.range = 30;
    s.pelletsPerShot = 8;
    s.headshotMult = 1.5f;
    s.isAutomatic = false;
    s.isExplosive = false;
    break;
  case WeaponType::SNIPER:
    s.name = "Barrett .50";
    s.damage = 120;
    s.fireRate = 0.5f;
    s.magSize = 5;
    s.reloadTime = 3.5f;
    s.accuracy = 0.99f;
    s.recoilAmount = 0.2f;
    s.range = 300;
    s.pelletsPerShot = 1;
    s.headshotMult = 3.0f;
    s.isAutomatic = false;
    s.isExplosive = false;
    break;
  case WeaponType::LMG:
    s.name = "M249 SAW";
    s.damage = 22;
    s.fireRate = 12;
    s.magSize = 100;
    s.reloadTime = 4.0f;
    s.accuracy = 0.70f;
    s.recoilAmount = 0.06f;
    s.range = 70;
    s.pelletsPerShot = 1;
    s.headshotMult = 2.0f;
    s.isAutomatic = true;
    s.isExplosive = false;
    break;
  case WeaponType::RPG:
    s.name = "RPG-7";
    s.damage = 200;
    s.fireRate = 0.3f;
    s.magSize = 1;
    s.reloadTime = 4.0f;
    s.accuracy = 0.95f;
    s.recoilAmount = 0.3f;
    s.range = 200;
    s.pelletsPerShot = 1;
    s.headshotMult = 1.0f;
    s.isAutomatic = false;
    s.isExplosive = true;
    s.projectileSpeed = 30.0f;
    s.blastRadius = 8.0f;
    break;
  case WeaponType::PISTOL:
    s.name = "Pistol";
    s.damage = 15;
    s.fireRate = 4.0f;
    s.magSize = 12;
    s.reloadTime = 1.5f;
    s.accuracy = 0.9f;
    s.recoilAmount = 0.05f;
    s.range = 50;
    s.pelletsPerShot = 1;
    s.headshotMult = 2.0f;
    s.isAutomatic = false;
    s.isExplosive = false;
    break;
  case WeaponType::AXE:
    s.name = "Axe";
    s.damage = 35;
    s.fireRate = 1.0f;
    s.magSize = 0;
    s.reloadTime = 0.0f;
    s.accuracy = 1.0f;
    s.recoilAmount = 0.0f;
    s.range = 2.5f;
    s.pelletsPerShot = 1;
    s.headshotMult = 1.0f;
    s.isAutomatic = false;
    s.isExplosive = false;
    break;
  default:
    break;
  }
  return s;
}

void WeaponSystemInit(WeaponSystem &ws) {
  int numWeapons = (int)WeaponType::COUNT; // include all weapons
  for (int i = 0; i < numWeapons; i++) {
    WeaponType t = (WeaponType)i;
    ws.weapons[i].type = t;
    ws.weapons[i].stats = GetDefaultStats(t);
    ws.weapons[i].currentAmmo = ws.weapons[i].stats.magSize;
    ws.weapons[i].reserveAmmo = ws.weapons[i].stats.magSize * 4;
    ws.weapons[i].fireCooldown = 0;
    ws.weapons[i].reloadTimer = 0;
    ws.weapons[i].isReloading = false;
    ws.weapons[i].recoilTimer = 0;
    ws.weapons[i].currentRecoil = 0;
    ws.weapons[i].attachments = {false, false, false, false};
  }
  ws.currentWeapon = 0;
  ws.justFired = false;
  ws.knifeAttacking = false;
  ws.knifeTimer = 0;
  ws.knifeCooldown = 0;
  ws.muzzleFlashPos = {0, 0, 0};
}

void WeaponSystemUpdate(WeaponSystem &ws, float dt) {
  Weapon &w = WeaponGetCurrent(ws);
  ws.justFired = false;

  if (w.fireCooldown > 0)
    w.fireCooldown -= dt;
  if (w.recoilTimer > 0) {
    w.recoilTimer -= dt;
    w.currentRecoil = w.stats.recoilAmount * (w.recoilTimer / 0.15f);
  } else {
    w.currentRecoil = 0;
  }

  if (w.isReloading) {
    w.reloadTimer -= dt;
    if (w.reloadTimer <= 0) {
      w.isReloading = false;
      int effective = WeaponGetEffectiveMagSize(w);
      int needed = effective - w.currentAmmo;
      int loaded = (w.reserveAmmo >= needed) ? needed : w.reserveAmmo;
      w.currentAmmo += loaded;
      w.reserveAmmo -= loaded;
    }
  }

  // Knife cooldown
  if (ws.knifeCooldown > 0)
    ws.knifeCooldown -= dt;
  if (ws.knifeTimer > 0) {
    ws.knifeTimer -= dt;
    ws.knifeAttacking = true;
  } else {
    ws.knifeAttacking = false;
  }
}

bool WeaponFire(WeaponSystem &ws) {
  Weapon &w = WeaponGetCurrent(ws);
  if (w.isReloading || w.fireCooldown > 0 || w.currentAmmo <= 0)
    return false;

  w.currentAmmo--;
  w.fireCooldown = 1.0f / w.stats.fireRate;
  w.recoilTimer = 0.15f;
  w.currentRecoil = w.stats.recoilAmount;
  ws.justFired = true;
  return true;
}

void WeaponReload(WeaponSystem &ws) {
  Weapon &w = WeaponGetCurrent(ws);
  int effective = WeaponGetEffectiveMagSize(w);
  if (w.isReloading || w.currentAmmo == effective || w.reserveAmmo <= 0)
    return;
  w.isReloading = true;
  w.reloadTimer = w.stats.reloadTime;
}

void WeaponSwitch(WeaponSystem &ws, int index) {
  int maxWeapons = (int)WeaponType::COUNT; // include all weapons
  if (index < 0 || index >= maxWeapons)
    return;
  if (index == ws.currentWeapon)
    return;
  ws.currentWeapon = index;
  ws.weapons[index].isReloading = false;
  ws.weapons[index].reloadTimer = 0;
}

Weapon &WeaponGetCurrent(WeaponSystem &ws) {
  return ws.weapons[ws.currentWeapon];
}

const Weapon &WeaponGetCurrentConst(const WeaponSystem &ws) {
  return ws.weapons[ws.currentWeapon];
}

bool WeaponKnifeAttack(WeaponSystem &ws) {
  if (ws.knifeCooldown > 0)
    return false;
  ws.knifeAttacking = true;
  ws.knifeTimer = 0.35f;
  ws.knifeCooldown = 0.8f;
  return true;
}

int WeaponGetEffectiveMagSize(const Weapon &w) {
  int base = w.stats.magSize;
  return w.attachments.hasExtendedMag ? (int)(base * 1.5f) : base;
}

float WeaponGetEffectiveAccuracy(const Weapon &w) {
  float acc = w.stats.accuracy;
  if (w.attachments.hasRedDot)
    acc = fminf(acc + 0.15f, 0.98f);
  return acc;
}

float WeaponGetEffectiveRecoil(const Weapon &w) {
  float r = w.stats.recoilAmount;
  if (w.attachments.hasForegrip)
    r *= 0.7f;
  return r;
}
