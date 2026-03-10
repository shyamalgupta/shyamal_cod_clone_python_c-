#include "audio_manager.h"
#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// --- Sound bank ---
static Sound sndGunshot;
static Sound sndShotgun;
static Sound sndSniper;
static Sound sndLMG;
static Sound sndRocketLaunch;
static Sound sndReload;
static Sound sndFootstep;
static Sound sndHitmarker;
static Sound sndDamage;
static Sound sndExplosion;
static Sound sndGrenadeBounce;
static Sound sndGrenadePin;
static Sound sndKillstreak;
static Sound sndWaveComplete;
static Sound sndHeartbeat;
static Sound sndAmmoPickup;
static Sound sndKnifeSwipe;
static Sound sndKnifeHit;
static Sound sndRankUp;
static Sound sndRain;

static bool audioReady = false;

// Procedural wave generator
static Wave GenerateWave(float freq, float duration, float amplitude,
                         int waveform, float decay, int sampleRate = 22050) {
  int sampleCount = (int)(sampleRate * duration);
  short *data = (short *)malloc(sampleCount * sizeof(short));
  for (int i = 0; i < sampleCount; i++) {
    float t = (float)i / sampleRate;
    float env = expf(-decay * t);
    float s = 0;
    switch (waveform) {
    case 0:
      s = sinf(2.0f * 3.14159f * freq * t);
      break; // sine
    case 1:
      s = (fmodf(freq * t, 1.0f) < 0.5f) ? 1.0f : -1.0f;
      break; // square
    case 2:
      s = 2.0f * fmodf(freq * t, 1.0f) - 1.0f;
      break; // sawtooth
    case 3:
      s = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
      break; // noise
    }
    data[i] = (short)(s * env * amplitude * 32767.0f);
  }
  Wave w = {};
  w.frameCount = sampleCount;
  w.sampleRate = sampleRate;
  w.sampleSize = 16;
  w.channels = 1;
  w.data = data;
  return w;
}

static Sound WaveToSound(Wave w) {
  Sound s = LoadSoundFromWave(w);
  UnloadWave(w);
  return s;
}

void AudioManagerInit() {
  InitAudioDevice();
  if (!IsAudioDeviceReady())
    return;

  // Gunshots
  sndGunshot = WaveToSound(GenerateWave(280, 0.18f, 0.6f, 1, 18));
  sndShotgun = WaveToSound(GenerateWave(120, 0.28f, 0.9f, 3, 12));
  sndSniper = WaveToSound(GenerateWave(200, 0.45f, 0.8f, 1, 8));
  sndLMG = WaveToSound(GenerateWave(300, 0.14f, 0.5f, 1, 20));
  sndRocketLaunch = WaveToSound(GenerateWave(80, 0.6f, 0.7f, 3, 5));
  // Other
  sndReload = WaveToSound(GenerateWave(600, 0.15f, 0.3f, 2, 25));
  sndFootstep = WaveToSound(GenerateWave(80, 0.12f, 0.3f, 3, 30));
  sndHitmarker = WaveToSound(GenerateWave(1800, 0.06f, 0.4f, 0, 40));
  sndDamage = WaveToSound(GenerateWave(150, 0.25f, 0.6f, 3, 12));
  sndExplosion = WaveToSound(GenerateWave(60, 0.8f, 0.9f, 3, 4));
  sndGrenadeBounce = WaveToSound(GenerateWave(400, 0.08f, 0.25f, 1, 40));
  sndGrenadePin = WaveToSound(GenerateWave(1200, 0.1f, 0.3f, 0, 30));
  sndKillstreak = WaveToSound(GenerateWave(880, 0.3f, 0.5f, 0, 8));
  sndWaveComplete = WaveToSound(GenerateWave(523, 0.5f, 0.6f, 0, 5));
  sndHeartbeat = WaveToSound(GenerateWave(60, 0.2f, 0.4f, 0, 10));
  sndAmmoPickup = WaveToSound(GenerateWave(1000, 0.12f, 0.35f, 0, 20));
  // New sounds
  sndKnifeSwipe = WaveToSound(GenerateWave(800, 0.1f, 0.3f, 2, 25));
  sndKnifeHit = WaveToSound(GenerateWave(200, 0.15f, 0.5f, 3, 15));
  sndRankUp = WaveToSound(GenerateWave(660, 0.6f, 0.7f, 0, 4));
  sndRain = WaveToSound(GenerateWave(200, 2.0f, 0.15f, 3, 0));

  audioReady = true;
}

void AudioManagerShutdown() {
  if (!audioReady)
    return;
  UnloadSound(sndGunshot);
  UnloadSound(sndShotgun);
  UnloadSound(sndSniper);
  UnloadSound(sndLMG);
  UnloadSound(sndRocketLaunch);
  UnloadSound(sndReload);
  UnloadSound(sndFootstep);
  UnloadSound(sndHitmarker);
  UnloadSound(sndDamage);
  UnloadSound(sndExplosion);
  UnloadSound(sndGrenadeBounce);
  UnloadSound(sndGrenadePin);
  UnloadSound(sndKillstreak);
  UnloadSound(sndWaveComplete);
  UnloadSound(sndHeartbeat);
  UnloadSound(sndAmmoPickup);
  UnloadSound(sndKnifeSwipe);
  UnloadSound(sndKnifeHit);
  UnloadSound(sndRankUp);
  UnloadSound(sndRain);
  CloseAudioDevice();
}

// Weapon-specific gunshot
void AudioPlayGunshot(int weaponIndex) {
  if (!audioReady)
    return;
  switch (weaponIndex) {
  case 0:
    SetSoundPitch(sndGunshot, 0.9f + (float)rand() / RAND_MAX * 0.2f);
    PlaySound(sndGunshot);
    break;
  case 1:
    PlaySound(sndShotgun);
    break;
  case 2:
    PlaySound(sndSniper);
    break;
  case 3:
    SetSoundPitch(sndLMG, 0.95f + (float)rand() / RAND_MAX * 0.1f);
    PlaySound(sndLMG);
    break;
  case 4:
    PlaySound(sndRocketLaunch);
    break;
  default:
    PlaySound(sndGunshot);
    break;
  }
}
void AudioPlayGunshot() {
  if (audioReady)
    PlaySound(sndGunshot);
}
void AudioPlayReload() {
  if (audioReady)
    PlaySound(sndReload);
}
void AudioPlayFootstep() {
  if (audioReady) {
    SetSoundPitch(sndFootstep, 0.8f + (float)rand() / RAND_MAX * 0.4f);
    PlaySound(sndFootstep);
  }
}
void AudioPlayHitmarker() {
  if (audioReady)
    PlaySound(sndHitmarker);
}
void AudioPlayDamage() {
  if (audioReady)
    PlaySound(sndDamage);
}
void AudioPlayExplosion() {
  if (audioReady)
    PlaySound(sndExplosion);
}
void AudioPlayGrenadeBounce() {
  if (audioReady)
    PlaySound(sndGrenadeBounce);
}
void AudioPlayGrenadePin() {
  if (audioReady)
    PlaySound(sndGrenadePin);
}
void AudioPlayKillstreak() {
  if (audioReady)
    PlaySound(sndKillstreak);
}
void AudioPlayWaveComplete() {
  if (audioReady)
    PlaySound(sndWaveComplete);
}
void AudioPlayHeartbeat() {
  if (audioReady)
    PlaySound(sndHeartbeat);
}
void AudioPlayAmmoPickup() {
  if (audioReady)
    PlaySound(sndAmmoPickup);
}
void AudioPlayKnifeSwipe() {
  if (audioReady)
    PlaySound(sndKnifeSwipe);
}
void AudioPlayKnifeHit() {
  if (audioReady)
    PlaySound(sndKnifeHit);
}
void AudioPlayRankUp() {
  if (audioReady)
    PlaySound(sndRankUp);
}
void AudioPlayRain() {
  if (audioReady) {
    if (!IsSoundPlaying(sndRain))
      PlaySound(sndRain);
  }
}
