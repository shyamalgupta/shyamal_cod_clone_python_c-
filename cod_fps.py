#!/usr/bin/env python3
"""
COD FPS — Call of Duty Style First Person Shooter
Python + OpenGL Edition  |  Hybrid with C++ engine (when compiled)

Controls:
  WASD       — Move
  Mouse      — Look
  Left Click — Shoot
  Right Click— Aim/Zoom
  1/2/3      — Switch weapons
  R          — Reload
  H          — Heal (5s cooldown)
  Shift      — Sprint
  Space      — Jump
  Enter      — Respawn
  ESC        — Pause/Quit
"""
import pygame
import math
import random
import sys
import os
import time as time_module
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
from game_engine import (
    Weapon, Enemy, AABB, ParticleSystem,
    draw_cube, draw_cube_textured, draw_ground, draw_cube_solid, draw_sphere,
    vec3_add, vec3_sub, vec3_scale, vec3_normalize, vec3_dist, vec3_length
)

# Load Python scripts
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'scripts'))
try:
    from weapons import get_weapons
    from ai_behavior import get_ai_params
    from events import on_kill, on_damage
    SCRIPTS_OK = True
    print("[Python Bridge] Scripts loaded successfully!")
except ImportError as e:
    SCRIPTS_OK = False
    print(f"[Python Bridge] Scripts not found ({e}), using defaults")

# ============================================================
# LEVEL DATA
# ============================================================
class Level:
    def __init__(self):
        self.ground_size = 120
        self.ground_color = (0.20, 0.23, 0.17)
        self.sky_color = (0.39, 0.55, 0.71)
        self.buildings = [
            {"pos": (0,0,25), "size": (12,6,8), "color": (0.35,0.35,0.33), "roof": (0.27,0.27,0.25)},
            {"pos": (-30,0,-20), "size": (10,5,10), "color": (0.39,0.33,0.29), "roof": (0.31,0.25,0.21)},
            {"pos": (35,0,30), "size": (6,10,6), "color": (0.33,0.35,0.37), "roof": (0.25,0.27,0.29)},
            {"pos": (30,0,-25), "size": (14,4,6), "color": (0.37,0.35,0.31), "roof": (0.29,0.27,0.23)},
            {"pos": (-35,0,35), "size": (8,3,8), "color": (0.31,0.33,0.31), "roof": (0.23,0.25,0.23)},
            {"pos": (45,0,5), "size": (10,4,8), "color": (0.35,0.31,0.27), "roof": (0.27,0.23,0.20)},
            {"pos": (-40,0,10), "size": (12,6,10), "color": (0.33,0.33,0.35), "roof": (0.25,0.25,0.27)},
            {"pos": (10,0,-40), "size": (8,5,8), "color": (0.37,0.33,0.31), "roof": (0.29,0.25,0.23)},
        ]
        self.walls = [
            {"pos": (-15,0,15), "size": (1,3,20), "color": (0.43,0.41,0.39)},
            {"pos": (15,0,15), "size": (1,3,20), "color": (0.43,0.41,0.39)},
            {"pos": (0,0,35), "size": (30,3,1), "color": (0.43,0.41,0.39)},
            {"pos": (-50,0,0), "size": (1,4,50), "color": (0.31,0.31,0.31)},
            {"pos": (50,0,0), "size": (1,4,50), "color": (0.31,0.31,0.31)},
            {"pos": (0,0,-50), "size": (50,4,1), "color": (0.31,0.31,0.31)},
            {"pos": (0,0,50), "size": (50,4,1), "color": (0.31,0.31,0.31)},
            {"pos": (-20,0,0), "size": (6,2.5,0.5), "color": (0.39,0.37,0.35)},
            {"pos": (20,0,-10), "size": (0.5,2.5,8), "color": (0.39,0.37,0.35)},
            {"pos": (5,0,-15), "size": (8,2,0.5), "color": (0.39,0.37,0.35)},
        ]
        self.covers = [
            {"pos": (3,0,5), "size": (1.5,1.2,1.5), "color": (0.47,0.35,0.20)},
            {"pos": (-5,0,8), "size": (2,1,1), "color": (0.35,0.33,0.27)},
            {"pos": (10,0,15), "size": (1.2,1,1.2), "color": (0.43,0.31,0.18)},
            {"pos": (-10,0,20), "size": (2,1.5,1), "color": (0.39,0.35,0.23)},
            {"pos": (0,0,-10), "size": (1.5,1,2), "color": (0.45,0.33,0.20)},
            {"pos": (25,0,10), "size": (1.5,1,1), "color": (0.41,0.37,0.27)},
            {"pos": (-25,0,-10), "size": (1,1.2,1.5), "color": (0.43,0.33,0.21)},
            {"pos": (15,0,-30), "size": (2,1,1), "color": (0.39,0.35,0.25)},
            {"pos": (-15,0,30), "size": (1.5,1.5,1.5), "color": (0.47,0.37,0.21)},
            {"pos": (35,0,-10), "size": (1,1,2), "color": (0.41,0.33,0.23)},
            {"pos": (-30,0,20), "size": (2,1,1.5), "color": (0.45,0.35,0.20)},
            {"pos": (40,0,20), "size": (1.5,1.2,1), "color": (0.39,0.31,0.21)},
        ]
        self.enemy_spawns = [
            (-25,0,-15),(25,0,-20),(30,0,25),(-30,0,30),(40,0,0),
            (-40,0,5),(5,0,-35),(-10,0,40),(20,0,15),(-20,0,-30),
        ]
        self.trees = [
            (-20,0,10), (15,0,-15), (20,0,30), (-35,0,-20), 
            (40,0,-25), (-10,0,-40), (5,0,35), (35,0,5)
        ]
        self.crates = [
            {"pos": (-5, 0.5, 5), "size": (1,1,1)},
            {"pos": (-5, 1.5, 5), "size": (1,1,1)}, # Stacked
            {"pos": (12, 0.5, -8), "size": (1,1,1)},
            {"pos": (-22, 0.5, -5), "size": (1,1,1)},
            {"pos": (18, 0.5, 20), "size": (1,1,1)},
            {"pos": (18, 1.5, 20), "size": (1,1,1)},
            {"pos": (19, 0.5, 20), "size": (1,1,1)}
        ]
        self.colliders = []
        for b in self.buildings:
            self.colliders.append(AABB(b["pos"], b["size"]))
        for w in self.walls:
            self.colliders.append(AABB(w["pos"], w["size"]))
        for c in self.covers:
            self.colliders.append(AABB(c["pos"], c["size"]))
        for c in self.crates:
            self.colliders.append(AABB(c["pos"], c["size"]))
        for t in self.trees:
            self.colliders.append(AABB(t, (1.0, 5.0, 1.0)))

    def draw(self, tex_grass=None, tex_wall=None, tex_concrete=None, tex_wood=None):
        draw_ground(self.ground_size, self.ground_color, tex=tex_grass)
        
        # Draw Trees
        from game_engine import draw_tree
        for t in self.trees:
            draw_tree(t)

        # Draw Crates
        for c in self.crates:
            draw_cube(c["pos"], c["size"], (0.6, 0.4, 0.2), tex=tex_wood)

        for b in self.buildings:
            draw_cube(b["pos"], b["size"], b["color"], tex=tex_wall)
            rp = (b["pos"][0], b["pos"][1]+b["size"][1], b["pos"][2])
            rs = (b["size"][0]+0.3, 0.3, b["size"][2]+0.3)
            # Use concrete texture for roofs, fallback color
            draw_cube(rp, rs, b["roof"], tex=tex_concrete)
            # Windows
            wy = b["pos"][1] + b["size"][1]*0.6
            for wx_off in range(-int(b["size"][0]/2)+1, int(b["size"][0]/2), 3):
                for side in [1, -1]:
                    wz = b["pos"][2] + side * (b["size"][2]/2 + 0.01)
                    glPushMatrix()
                    glTranslatef(b["pos"][0]+wx_off, wy, wz)
                    glScalef(1.0, 1.2, 0.02)
                    glColor3f(0.12, 0.14, 0.16)
                    draw_cube_solid(tex=None)
                    glPopMatrix()
        for w in self.walls:
            draw_cube(w["pos"], w["size"], w["color"], tex=tex_wall)
        for c in self.covers:
            # Maybe use concrete for cover objects, or wall
            draw_cube(c["pos"], c["size"], c["color"], tex=tex_wall)

# ============================================================
# PLAYER
# ============================================================
class Player:
    def __init__(self):
        self.pos = [0.0, 0.0, 0.0]
        self.vel_y = 0; self.yaw = 0; self.pitch = 0
        self.health = 100; self.max_health = 100
        self.speed = 8.0; self.sprint_mult = 1.6
        self.is_grounded = True; self.is_sprinting = False
        self.height = 1.7; self.radius = 0.4
        self.kills = 0; self.deaths = 0
        self.damage_flash = 0; self.bob_timer = 0; self.bob_amount = 0
        # Healing system
        self.heal_cooldown = 0       # Time until next heal available
        self.heal_cooldown_max = 5.0 # 5 second cooldown
        self.heal_amount = 40        # Heal 40 HP per use
        self.heal_flash = 0          # Green flash when healing
        self.is_healing = False      # Currently in heal animation
        self.heal_timer = 0          # Heal animation timer
        # Passive regen
        self.regen_delay = 4.0       # Seconds without damage before regen starts
        self.regen_timer = 0         # Time since last damage
        self.regen_rate = 3.0        # HP per second during regen

    def update(self, dt, keys, colliders):
        forward = (math.sin(self.yaw), 0, math.cos(self.yaw))
        right = (math.cos(self.yaw), 0, -math.sin(self.yaw))
        move = [0, 0, 0]
        if keys[K_w]: move = vec3_add(move, forward)
        if keys[K_s]: move = vec3_sub(move, forward)
        if keys[K_a]: move = vec3_sub(move, right)
        if keys[K_d]: move = vec3_add(move, right)
        move = (move[0], 0, move[2])
        ml = vec3_length(move)
        if ml > 0.01: move = vec3_normalize(move)
        
        self.is_sprinting = keys[K_LSHIFT] and keys[K_w]
        spd = self.speed * (self.sprint_mult if self.is_sprinting else 1.0)
        
        new_pos = list(self.pos)
        new_pos[0] += move[0] * spd * dt
        new_pos[2] += move[2] * spd * dt
        
        # Collision X
        test = AABB((new_pos[0], self.pos[1], self.pos[2]), (self.radius*2, self.height, self.radius*2))
        can_x = True
        for c in colliders:
            if self._aabb_overlap(test, c): can_x = False; break
        if can_x: self.pos[0] = new_pos[0]
        
        # Collision Z
        test = AABB((self.pos[0], self.pos[1], new_pos[2]), (self.radius*2, self.height, self.radius*2))
        can_z = True
        for c in colliders:
            if self._aabb_overlap(test, c): can_z = False; break
        if can_z: self.pos[2] = new_pos[2]

        # Gravity / Jump
        if not self.is_grounded:
            self.vel_y -= 15 * dt
            self.pos[1] += self.vel_y * dt
        if self.pos[1] <= 0:
            self.pos[1] = 0; self.vel_y = 0; self.is_grounded = True

        # Boundaries
        bnd = 58
        self.pos[0] = max(-bnd, min(bnd, self.pos[0]))
        self.pos[2] = max(-bnd, min(bnd, self.pos[2]))

        # Head bob
        if ml > 0.01 and self.is_grounded:
            self.bob_timer += dt * (14 if self.is_sprinting else 10)
            self.bob_amount = math.sin(self.bob_timer) * 0.05
        else:
            self.bob_timer = 0; self.bob_amount *= 0.9

        if self.damage_flash > 0: self.damage_flash -= dt
        if self.heal_flash > 0: self.heal_flash -= dt
        if self.heal_cooldown > 0: self.heal_cooldown -= dt

        # Heal animation
        if self.is_healing:
            self.heal_timer -= dt
            if self.heal_timer <= 0:
                self.is_healing = False
                self.health = min(self.health + self.heal_amount, self.max_health)
                self.heal_flash = 0.5
                print(f"[HEAL] +{self.heal_amount} HP -> {self.health:.0f}/{self.max_health}")

        # Passive regen (after not taking damage for a while)
        self.regen_timer += dt
        if self.regen_timer >= self.regen_delay and self.health < self.max_health and self.health > 0:
            self.health = min(self.health + self.regen_rate * dt, self.max_health)

    def try_heal(self):
        """Try to use heal ability. Returns True if successful."""
        if self.heal_cooldown > 0: return False
        if self.health >= self.max_health: return False
        if not self.alive(): return False
        if self.is_healing: return False
        self.is_healing = True
        self.heal_timer = 0.8  # 0.8 second heal animation
        self.heal_cooldown = self.heal_cooldown_max
        return True

    def _aabb_overlap(self, a, b):
        return (a.min[0]<=b.max[0] and a.max[0]>=b.min[0] and
                a.min[1]<=b.max[1] and a.max[1]>=b.min[1] and
                a.min[2]<=b.max[2] and a.max[2]>=b.min[2])

    def get_cam_pos(self):
        return (self.pos[0], self.pos[1]+self.height+self.bob_amount, self.pos[2])

    def get_look_dir(self):
        return (math.cos(self.pitch)*math.sin(self.yaw),
                math.sin(self.pitch),
                math.cos(self.pitch)*math.cos(self.yaw))

    def take_damage(self, dmg):
        self.health -= dmg
        self.damage_flash = 0.3
        self.regen_timer = 0  # Reset regen timer on damage
        if self.health <= 0:
            self.health = 0; self.deaths += 1

    def respawn(self):
        self.pos = [0,0,0]; self.vel_y = 0
        self.health = self.max_health; self.is_grounded = True
        self.damage_flash = 0; self.heal_cooldown = 0
        self.is_healing = False

    def alive(self): return self.health > 0

# ============================================================
# HUD DRAWING (2D overlay)
# ============================================================
def setup_2d(w, h):
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity()
    glOrtho(0, w, h, 0, -1, 1)
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity()
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING)

def end_2d():
    glEnable(GL_DEPTH_TEST)
    glMatrixMode(GL_PROJECTION); glPopMatrix()
    glMatrixMode(GL_MODELVIEW); glPopMatrix()

def draw_rect(x, y, w, h, color):
    glColor4f(*color)
    glBegin(GL_QUADS)
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h)
    glEnd()

def draw_rect_outline(x, y, w, h, color):
    glColor4f(*color)
    glBegin(GL_LINE_LOOP)
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h)
    glEnd()

class HUD:
    def __init__(self):
        pygame.font.init()
        self.font_large = pygame.font.SysFont('consolas', 30, bold=True)
        self.font_med = pygame.font.SysFont('consolas', 18, bold=True)
        self.font_small = pygame.font.SysFont('consolas', 13)
        self.font_title = pygame.font.SysFont('consolas', 50, bold=True)
        self.hitmarker_timer = 0
        self.kill_feed = []  # (message, timer, color)
        self._text_cache = {}

    def update(self, dt):
        if self.hitmarker_timer > 0: self.hitmarker_timer -= dt
        self.kill_feed = [(m,t-dt,c) for m,t,c in self.kill_feed if t-dt > 0]

    def add_kill(self, msg, color=(1,0.3,0.3)):
        self.kill_feed.append((msg, 4.0, color))
        if len(self.kill_feed) > 5: self.kill_feed.pop(0)

    def trigger_hitmarker(self):
        self.hitmarker_timer = 0.2

    def _render_text(self, font, text, color):
        key = (id(font), text, color)
        if key not in self._text_cache:
            surf = font.render(text, True, color)
            w, h = surf.get_size()
            data = pygame.image.tostring(surf, "RGBA", True)
            tex = glGenTextures(1)
            glBindTexture(GL_TEXTURE_2D, tex)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            self._text_cache[key] = (tex, w, h)
        return self._text_cache[key]

    def draw_text(self, font, text, x, y, color=(255,255,255)):
        tex, w, h = self._render_text(font, text, color)
        glEnable(GL_TEXTURE_2D)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        glBindTexture(GL_TEXTURE_2D, tex)
        glColor4f(1,1,1,1)
        glBegin(GL_QUADS)
        glTexCoord2f(0,0); glVertex2f(x, y+h)
        glTexCoord2f(1,0); glVertex2f(x+w, y+h)
        glTexCoord2f(1,1); glVertex2f(x+w, y)
        glTexCoord2f(0,1); glVertex2f(x, y)
        glEnd()
        glDisable(GL_TEXTURE_2D)
        return w, h

    def draw(self, screen_w, screen_h, player, weapons, enemies, is_aiming):
        setup_2d(screen_w, screen_h)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

        cur_w = weapons[0]  # current weapon ref passed in
        cx, cy = screen_w//2, screen_h//2

        # Crosshair
        if not is_aiming or weapons[1] != 2:  # not sniper scope
            gap = 3 if is_aiming else 6; ln = 8 if is_aiming else 12
            draw_rect(cx-ln-gap, cy-1, ln, 2, (0.78,1,0.78,0.8))
            draw_rect(cx+gap, cy-1, ln, 2, (0.78,1,0.78,0.8))
            draw_rect(cx-1, cy-ln-gap, 2, ln, (0.78,1,0.78,0.8))
            draw_rect(cx-1, cy+gap, 2, ln, (0.78,1,0.78,0.8))
            draw_rect(cx-1, cy-1, 2, 2, (1,1,1,0.9))

        # Hitmarker
        if self.hitmarker_timer > 0:
            a = self.hitmarker_timer / 0.2
            s = 10
            glColor4f(1,1,1,a); glLineWidth(2)
            glBegin(GL_LINES)
            for dx,dy in [(-1,-1),(1,-1),(1,1),(-1,1)]:
                glVertex2f(cx+dx*s, cy+dy*s); glVertex2f(cx+dx*s//2, cy+dy*s//2)
            glEnd(); glLineWidth(1)

        # Health bar
        hp_pct = player.health / player.max_health
        draw_rect(28, screen_h-52, 204, 22, (0,0,0,0.6))
        draw_rect(30, screen_h-50, 200, 18, (0.16,0.16,0.16,0.8))
        hc = (0.2,0.78,0.31) if hp_pct > 0.6 else ((0.86,0.71,0.16) if hp_pct > 0.3 else (0.86,0.2,0.2))
        draw_rect(30, screen_h-50, int(200*hp_pct), 18, (*hc, 1))
        self.draw_text(self.font_small, f"HP {player.health:.0f}", 35, screen_h-50, (255,255,255))
        self.draw_text(self.font_med, "+", 10, screen_h-53, tuple(int(c*255) for c in hc))

        # ======= HEAL INDICATOR =======
        heal_x = 240; heal_y = screen_h - 52
        if player.heal_cooldown > 0:
            # On cooldown — show grey bar with countdown
            cd_pct = player.heal_cooldown / player.heal_cooldown_max
            draw_rect(heal_x, heal_y, 82, 22, (0,0,0,0.5))
            draw_rect(heal_x+1, heal_y+1, 80, 20, (0.15,0.15,0.15,0.7))
            draw_rect(heal_x+1, heal_y+1, int(80*(1-cd_pct)), 20, (0.2,0.4,0.2,0.7))
            self.draw_text(self.font_small, f"H:{player.heal_cooldown:.1f}s", heal_x+5, heal_y+3, (150,150,150))
        elif player.health < player.max_health:
            # Ready to heal — pulsing green
            pulse = 0.6 + 0.4 * math.sin(pygame.time.get_ticks() * 0.005)
            draw_rect(heal_x, heal_y, 82, 22, (0,0,0,0.5))
            draw_rect(heal_x+1, heal_y+1, 80, 20, (0.1*pulse, 0.5*pulse, 0.15*pulse, 0.9))
            self.draw_text(self.font_small, "[H] HEAL", heal_x+5, heal_y+3, (100,255,120))
        else:
            # Full health
            draw_rect(heal_x, heal_y, 82, 22, (0,0,0,0.3))
            draw_rect(heal_x+1, heal_y+1, 80, 20, (0.1,0.2,0.1,0.5))
            self.draw_text(self.font_small, "[H] FULL", heal_x+5, heal_y+3, (80,150,80))

        # Healing animation (inject indicator)
        if player.is_healing:
            heal_pct = 1.0 - player.heal_timer / 0.8
            draw_rect(cx-75, cy+60, 150, 8, (0.1,0.1,0.1,0.7))
            draw_rect(cx-75, cy+60, int(150*heal_pct), 8, (0.2,0.9,0.3,0.9))
            self.draw_text(self.font_small, "HEALING...", cx-30, cy+50, (100,255,120))

        # Ammo
        draw_rect(screen_w-210, screen_h-60, 195, 52, (0,0,0,0.55))
        self.draw_text(self.font_large, f"{cur_w.current_ammo}", screen_w-200, screen_h-55, (255,255,255))
        self.draw_text(self.font_med, f"/ {cur_w.reserve_ammo}", screen_w-135, screen_h-45, (180,180,180))
        self.draw_text(self.font_small, cur_w.name, screen_w-200, screen_h-22, (150,200,150))
        if cur_w.is_reloading:
            pct = 1.0 - cur_w.reload_timer / cur_w.reload_time
            draw_rect(screen_w-200, screen_h-68, 150, 6, (0.16,0.16,0.16,0.8))
            draw_rect(screen_w-200, screen_h-68, int(150*pct), 6, (0.4,0.78,1,1))
            self.draw_text(self.font_small, "RELOADING", screen_w-160, screen_h-70, (100,200,255))

        # Weapon selector
        names = ["M4A1","SPAS","BARR"]
        bw, bh = 65, 25
        sx = screen_w//2 - (3*bw+10)//2
        for i in range(3):
            x = sx + i*(bw+5)
            active = (i == weapons[1])
            bg = (0.23,0.47,0.23,0.8) if active else (0.16,0.16,0.16,0.6)
            brd = (0.39,0.86,0.39,1) if active else (0.31,0.31,0.31,0.6)
            draw_rect(x, screen_h-92, bw, bh, bg)
            draw_rect_outline(x, screen_h-92, bw, bh, brd)
            self.draw_text(self.font_small, f"{i+1}", x+3, screen_h-90, (180,180,180))
            tc = (255,255,255) if active else (150,150,150)
            self.draw_text(self.font_small, names[i], x+15, screen_h-88, tc)

        # Kill counter + K/D ratio
        kd = player.kills / max(player.deaths, 1)
        self.draw_text(self.font_med, f"KILLS: {player.kills}  DEATHS: {player.deaths}  K/D: {kd:.1f}", 30, 20, (200,220,200))

        # FPS
        fps = int(self._fps) if hasattr(self, '_fps') else 0
        self.draw_text(self.font_small, f"{fps} FPS", 30, 45, (150,150,150))

        # Kill feed
        for i, (msg, timer, color) in enumerate(self.kill_feed):
            a = min(timer/4.0, 1.0)
            c = tuple(int(cc*255*a) for cc in color)
            self.draw_text(self.font_small, msg, screen_w-350, 190+i*22, c)

        # Minimap
        ms = 160; mx = screen_w-ms-15; my = 15
        scale = ms / 120
        draw_rect(mx-2, my-2, ms+4, ms+4, (0,0,0,0.7))
        draw_rect(mx, my, ms, ms, (0.08,0.12,0.08,0.8))
        mcx, mcy = mx+ms//2, my+ms//2
        # Buildings on minimap
        for b in [{"pos":bp["pos"],"size":bp["size"]} for bp in 
            [{"pos":(0,0,25),"size":(12,6,8)},{"pos":(-30,0,-20),"size":(10,5,10)},
             {"pos":(35,0,30),"size":(6,10,6)},{"pos":(30,0,-25),"size":(14,4,6)},
             {"pos":(-35,0,35),"size":(8,3,8)},{"pos":(45,0,5),"size":(10,4,8)},
             {"pos":(-40,0,10),"size":(12,6,10)},{"pos":(10,0,-40),"size":(8,5,8)}]]:
            bx = mcx + int(b["pos"][0]*scale)
            by = mcy - int(b["pos"][2]*scale)
            bw2 = max(int(b["size"][0]*scale), 1)
            bh2 = max(int(b["size"][2]*scale), 1)
            draw_rect(bx-bw2//2, by-bh2//2, bw2, bh2, (0.31,0.31,0.27,0.7))
        # Enemies
        for e in enemies:
            if e.state == Enemy.DEAD: continue
            ex = mcx + int(e.pos[0]*scale)
            ey = mcy - int(e.pos[2]*scale)
            if mx<=ex<=mx+ms and my<=ey<=my+ms:
                draw_rect(ex-2, ey-2, 4, 4, (1,0.23,0.23,0.86))
        # Player
        px = mcx + int(player.pos[0]*scale)
        py = mcy - int(player.pos[2]*scale)
        draw_rect(px-3, py-3, 6, 6, (0.23,1,0.23,1))
        # Player direction
        ddx = math.sin(player.yaw)*8; ddy = -math.cos(player.yaw)*8
        glColor4f(0.23,1,0.23,1); glBegin(GL_LINES)
        glVertex2f(px, py); glVertex2f(px+ddx, py+ddy); glEnd()
        draw_rect_outline(mx-2, my-2, ms+4, ms+4, (0.39,0.78,0.39,0.6))
        self.draw_text(self.font_small, "MAP", mx+4, my+2, (100,200,100))

        # Damage vignette (red)
        if player.damage_flash > 0:
            a = player.damage_flash / 0.3
            draw_rect(0, 0, screen_w, screen_h, (0.78,0,0,0.39*a))
            draw_rect(0, 0, 40, screen_h, (0.78,0,0,0.23*a))
            draw_rect(screen_w-40, 0, 40, screen_h, (0.78,0,0,0.23*a))

        # Heal vignette (green)
        if player.heal_flash > 0:
            a = player.heal_flash / 0.5
            draw_rect(0, 0, screen_w, screen_h, (0,0.6,0.15,0.2*a))
            draw_rect(0, 0, 30, screen_h, (0,0.7,0.2,0.15*a))
            draw_rect(screen_w-30, 0, 30, screen_h, (0,0.7,0.2,0.15*a))

        # Death screen
        if not player.alive():
            draw_rect(0, 0, screen_w, screen_h, (0,0,0,0.6))
            self.draw_text(self.font_title, "YOU DIED", cx-120, cy-40, (200,30,30))
            self.draw_text(self.font_med, "Press ENTER to respawn", cx-130, cy+30, (200,200,200))

        # Sniper scope overlay
        if is_aiming and weapons[1] == 2 and player.alive():
            sr = 200
            draw_rect(0, 0, cx-sr, screen_h, (0,0,0,1))
            draw_rect(cx+sr, 0, cx-sr, screen_h, (0,0,0,1))
            draw_rect(cx-sr, 0, sr*2, cy-sr, (0,0,0,1))
            draw_rect(cx-sr, cy+sr, sr*2, cy-sr, (0,0,0,1))
            glColor4f(0,0,0,0.7); glLineWidth(1)
            glBegin(GL_LINES)
            glVertex2f(cx-sr, cy); glVertex2f(cx+sr, cy)
            glVertex2f(cx, cy-sr); glVertex2f(cx, cy+sr)
            glEnd()
            for i in range(1,5):
                glBegin(GL_LINES)
                glVertex2f(cx-10, cy+i*30); glVertex2f(cx+10, cy+i*30)
                glEnd()

        glDisable(GL_BLEND)
        end_2d()

# ============================================================
# MAIN GAME
# ============================================================
def main():
    pygame.init()
    screen_w, screen_h = 1280, 720
    pygame.display.set_mode((screen_w, screen_h), DOUBLEBUF | OPENGL | HWSURFACE)
    pygame.display.set_caption("COD FPS — Call of Duty Style Shooter [Python + C++ Hybrid Engine]")
    pygame.mouse.set_visible(False)
    pygame.event.set_grab(True)

    # OpenGL setup
    glEnable(GL_DEPTH_TEST)
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    # Match clear color with fog color for seamless transition
    FOG_COLOR = (0.35, 0.45, 0.55, 1.0)
    glClearColor(*FOG_COLOR)

    # Enable OpenGL fog for amazing atmospheric depth
    glEnable(GL_FOG)
    glFogi(GL_FOG_MODE, GL_EXP2)
    from ctypes import c_float
    fog_color_arr = (c_float * 4)(*FOG_COLOR)
    glFogfv(GL_FOG_COLOR, fog_color_arr)
    glFogf(GL_FOG_DENSITY, 0.02)  # Visibility range ~50 units
    glHint(GL_FOG_HINT, GL_NICEST)

    # Enable OpenGL lighting for better visuals
    glEnable(GL_LIGHTING)
    glEnable(GL_LIGHT0)
    glEnable(GL_COLOR_MATERIAL)
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)
    # Sun light from above-right
    light_pos = (c_float * 4)(0.5, 1.0, 0.3, 0.0)  # directional
    light_amb = (c_float * 4)(0.35, 0.35, 0.4, 1.0)
    light_dif = (c_float * 4)(0.8, 0.75, 0.65, 1.0)
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos)
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb)
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_dif)

    from game_engine import generate_texture
    tex_grass = generate_texture("grass")
    tex_wall = generate_texture("wall")
    tex_concrete = generate_texture("concrete")
    tex_wood = generate_texture("wood")
    if tex_grass:
        # Use GL_LINEAR_MIPMAP_LINEAR for high-res procedural textures
        glBindTexture(GL_TEXTURE_2D, tex_grass)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glBindTexture(GL_TEXTURE_2D, tex_wall)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glBindTexture(GL_TEXTURE_2D, tex_concrete)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)

    # Init Multiplayer Network
    from network import Network
    net = Network()
    print(f"[NETWORK] Connected as Player ID: {net.id}" if net.id else "[NETWORK] Offline Mode")

    # Init systems
    level = Level()
    player = Player()
    particles = ParticleSystem()
    hud = HUD()

    # Load weapons from Python config
    if SCRIPTS_OK:
        wp_data = get_weapons()
        weapon_list = [
            Weapon(w["name"], w["damage"], w["fire_rate"], w["mag_size"],
                   w["reload_time"], w["accuracy"], w["recoil"], w["range"],
                   w.get("pellets", 1))
            for w in wp_data
        ]
    else:
        weapon_list = [
            Weapon("M4A1", 28, 10, 30, 2.2, 0.92, 0.03, 100),
            Weapon("SPAS-12", 18, 1.5, 8, 3.0, 0.7, 0.12, 25, 8),
            Weapon("BARRETT .50", 95, 0.8, 5, 3.5, 0.98, 0.2, 200),
        ]
    current_weapon = 0

    # Load AI params
    ai_params = None
    if SCRIPTS_OK:
        ai_params = get_ai_params()

    # Init enemies
    enemies = [Enemy(sp, ai_params) for sp in level.enemy_spawns[:10]]

    # Audio init
    try:
        pygame.mixer.init(44100, -16, 1, 512)
        import numpy as np
        HAS_AUDIO = True
    except:
        HAS_AUDIO = False

    def gen_sound(duration, freq, decay, noise=0.5):
        if not HAS_AUDIO: return None
        sr = 44100; n = int(sr * duration)
        t = np.linspace(0, duration, n, False)
        env = np.exp(-t * decay)
        sig = np.sin(2*np.pi*freq*t) * (1-noise) + np.random.uniform(-1,1,n) * noise
        sig = (sig * env * 30000).astype(np.int16)
        return pygame.mixer.Sound(buffer=sig.tobytes())

    snd_guns = [gen_sound(0.15,150,30,0.7), gen_sound(0.25,80,15,0.9), gen_sound(0.3,200,20,0.5)]
    snd_hit = gen_sound(0.08, 2000, 50, 0.1)
    snd_kill = gen_sound(0.3, 600, 10, 0.2)
    snd_dmg = gen_sound(0.15, 200, 25, 0.7)
    snd_heal = gen_sound(0.4, 800, 8, 0.15)  # Healing sound (rising tone)

    clock = pygame.time.Clock()
    hud._clock = clock
    is_aiming = False
    paused = False
    footstep_timer = 0

    print("\n" + "="*50)
    print("  COD FPS — GAME STARTED!")
    print("  WASD=Move  Mouse=Look  Click=Shoot")
    print("  1/2/3=Weapons  R=Reload  H=Heal")
    print("  Shift=Sprint  Space=Jump")
    print("  ENEMIES NERFED — You are the predator!")
    print("="*50 + "\n")

    running = True
    while running:
        dt = clock.tick(60) / 1000.0
        if dt > 0.05: dt = 0.05

        # Events
        for event in pygame.event.get():
            if event.type == QUIT:
                running = False
            elif event.type == KEYDOWN:
                if event.key == K_ESCAPE:
                    if paused:
                        running = False
                    else:
                        paused = True
                if paused and event.key == K_RETURN:
                    paused = False
                    pygame.mouse.set_pos(screen_w//2, screen_h//2)
                if not paused and player.alive():
                    if event.key == K_1: current_weapon = 0
                    elif event.key == K_2: current_weapon = 1
                    elif event.key == K_3: current_weapon = 2
                    elif event.key == K_r: weapon_list[current_weapon].reload()
                    elif event.key == K_h:
                        if player.try_heal():
                            if snd_heal: snd_heal.play()
                            hud.add_kill(f"Healing +{player.heal_amount} HP", (0.2,1,0.4))
                    elif event.key == K_SPACE and player.is_grounded:
                        player.vel_y = 6; player.is_grounded = False
                if not player.alive() and event.key == K_RETURN:
                    player.respawn()
                    weapon_list[current_weapon].current_ammo = weapon_list[current_weapon].mag_size

        if paused:
            # Draw pause screen
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
            setup_2d(screen_w, screen_h)
            glEnable(GL_BLEND)
            draw_rect(0, 0, screen_w, screen_h, (0,0,0,0.6))
            hud.draw_text(hud.font_title, "PAUSED", screen_w//2-90, screen_h//2-50, (200,200,200))
            hud.draw_text(hud.font_med, "ENTER=Resume  ESC=Quit", screen_w//2-140, screen_h//2+20, (150,150,150))
            glDisable(GL_BLEND)
            end_2d()
            pygame.display.flip()
            continue

        # Mouse look
        if player.alive():
            mx, my = pygame.mouse.get_rel()
            sens = 0.003
            player.yaw -= mx * sens
            player.pitch -= my * sens
            player.pitch = max(-1.4, min(1.4, player.pitch))

        # Aiming
        is_aiming = pygame.mouse.get_pressed()[2]
        target_fov = 40 if is_aiming else 75

        # Update player
        keys = pygame.key.get_pressed()
        if player.alive():
            player.update(dt, keys, level.colliders)
            # Sync to server
            if net.id:
                net.send_update({
                    "x": player.pos[0], "y": player.pos[1], "z": player.pos[2],
                    "yaw": player.yaw, "pitch": player.pitch,
                    "anim_time": player.bob_timer,
                    "weapon": current_weapon,
                    "health": player.health,
                    "is_dead": not player.alive()
                })
        
        # Process Server Events (kills, etc)
        if net.id:
            for ev in net.latest_state.get("events", []):
                if ev["type"] == "shoot" and ev["shooter"] != net.id:
                    # Could play 3D positional sound here
                    pass
                elif ev["type"] == "hit":
                    if ev["target"] == net.id:
                        player.take_damage(ev["damage"])
                        if snd_dmg: snd_dmg.play()
                        if SCRIPTS_OK: on_damage(ev["damage"], player.health)
                    elif ev["shooter"] == net.id and ev["fatal"]:
                        player.kills += 1
                        hud.add_kill("Player eliminated!")
                        if snd_kill: snd_kill.play()
                        if SCRIPTS_OK: on_kill(player.kills)

        # Update weapon
        w = weapon_list[current_weapon]
        w.update(dt)

        # Shooting
        if player.alive():
            want_shoot = False
            if w.fire_rate > 3:
                want_shoot = pygame.mouse.get_pressed()[0]
            else:
                # Semi auto — check via event (simplified: use key state + cooldown)
                want_shoot = pygame.mouse.get_pressed()[0]

            if want_shoot and w.fire():
                if snd_guns[current_weapon]: snd_guns[current_weapon].play()

                cam_pos = player.get_cam_pos()
                cam_dir = player.get_look_dir()
                # Inaccuracy
                inacc = (1-w.accuracy) * (0.3 if is_aiming else 1.0)
                cam_dir = vec3_normalize((
                    cam_dir[0] + random.uniform(-0.5,0.5)*inacc*0.1,
                    cam_dir[1] + random.uniform(-0.5,0.5)*inacc*0.1,
                    cam_dir[2] + random.uniform(-0.5,0.5)*inacc*0.1))

                muz = vec3_add(cam_pos, vec3_scale(cam_dir, 1.5))
                muz = (muz[0], muz[1]-0.3, muz[2])
                particles.spawn_muzzle(muz, cam_dir)

                for _ in range(w.pellets):
                    pd = cam_dir
                    if w.pellets > 1:
                        pd = vec3_normalize((
                            pd[0]+random.uniform(-0.04,0.04),
                            pd[1]+random.uniform(-0.03,0.03),
                            pd[2]+random.uniform(-0.04,0.04)))

                    # Check enemy hits (Local AI)
                    best_dist = w.range; best_enemy = None; best_remote = None
                    for e in enemies:
                        if e.state == Enemy.DEAD: continue
                        box = e.get_aabb()
                        t = box.ray_intersect(cam_pos, pd, w.range)
                        if t and t < best_dist:
                            best_dist = t; best_enemy = e
                            
                    # Check enemy hits (Remote Multiplayer)
                    if net.id:
                        for pid_str, pdata in net.latest_state.get("players", {}).items():
                            pid = int(pid_str)
                            if pid == net.id or pdata.get("is_dead", True): continue
                            # Remote player AABB
                            r_pos = (pdata["x"], pdata["y"], pdata["z"])
                            box = AABB(r_pos, (0.7, 1.8, 0.5))
                            t = box.ray_intersect(cam_pos, pd, w.range)
                            if t and t < best_dist:
                                best_dist = t; best_enemy = None; best_remote = pid

                    if best_enemy or best_remote:
                        hit_pt = vec3_add(cam_pos, vec3_scale(pd, best_dist))
                        dmg = w.damage
                        if best_enemy and hit_pt[1] > best_enemy.pos[1] + 1.4: dmg *= 1.5
                        
                        particles.spawn_blood(hit_pt)
                        hud.trigger_hitmarker()
                        if snd_hit: snd_hit.play()
                        
                        if best_remote:
                            # Send client-authoritative hit
                            net.send_hit(best_remote, dmg)
                        else:
                            killed = best_enemy.take_damage(dmg)
                            if killed:
                                player.kills += 1
                                hud.add_kill("Enemy eliminated!")
                                if snd_kill: snd_kill.play()
                                if SCRIPTS_OK: on_kill(player.kills)
                    else:
                        # Wall hit
                        best_wall_dist = w.range
                        for col in level.colliders:
                            t = col.ray_intersect(cam_pos, pd, w.range)
                            if t and t < best_wall_dist:
                                best_wall_dist = t
                        if best_wall_dist < w.range:
                            hit_pt = vec3_add(cam_pos, vec3_scale(pd, best_wall_dist))
                            particles.spawn_impact(hit_pt)

                # Recoil
                player.pitch += w.recoil * (0.5 if is_aiming else 1.0)

        # Update enemies
        for e in enemies:
            hit_player = e.update(player.pos, dt, level.colliders)
            if hit_player and player.alive():
                player.take_damage(e.damage)
                if snd_dmg: snd_dmg.play()
                if SCRIPTS_OK: on_damage(e.damage, player.health)

        # Update particles and HUD
        particles.update(dt)
        hud.update(dt)
        hud._fps = clock.get_fps()

        # ===== RENDER =====
        glClearColor(*FOG_COLOR)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

        # 3D Camera
        glMatrixMode(GL_PROJECTION); glLoadIdentity()
        fov = target_fov
        gluPerspective(fov, screen_w/screen_h, 0.1, 500)
        glMatrixMode(GL_MODELVIEW); glLoadIdentity()

        cam = player.get_cam_pos()
        look = player.get_look_dir()
        target = vec3_add(cam, look)
        gluLookAt(cam[0],cam[1],cam[2], target[0],target[1],target[2], 0,1,0)

        # Update light position in view space
        glEnable(GL_LIGHTING)
        glEnable(GL_COLOR_MATERIAL)

        # Draw level
        level.draw(tex_grass=tex_grass, tex_wall=tex_wall, tex_concrete=tex_concrete, tex_wood=tex_wood)

        # Draw remote multiplayer opponents
        if net.id:
            for pid_str, pdata in net.latest_state.get("players", {}).items():
                pid = int(pid_str)
                if pid == net.id or pdata.get("is_dead", True): continue
                # We can reuse the Enemy class's awesome procedural humanoid rendering by mapping data to a dummy enemy
                dummy = Enemy([pdata["x"], pdata["y"], pdata["z"]])
                dummy.yaw = pdata["yaw"]
                dummy.walk_time = pdata.get("anim_time", 0)
                dummy.color = (0.8, 0.2, 0.2) # Red for enemies
                # Assign weapon to look like what they have equipped
                # The dummy render has an assault rifle by default.
                dummy.draw()

        # Draw local AI enemies
        for e in enemies: e.draw()

        # Draw particles (no lighting for bright emissive particles)
        glDisable(GL_LIGHTING)
        particles.draw()
        glEnable(GL_LIGHTING)

        # Draw weapon model (disable lighting for consistent look)
        glDisable(GL_LIGHTING)
        if player.alive():
            glPushMatrix()
            right = (math.cos(player.yaw), 0, -math.sin(player.yaw))
            up = (0, 1, 0)
            fwd = look
            wp = vec3_add(cam, vec3_scale(fwd, 0.8))
            bob = math.sin(player.bob_timer*0.5)*0.02
            offset_r = 0.0 if is_aiming else 0.25
            wp = vec3_add(wp, vec3_scale(right, offset_r))
            wp = vec3_add(wp, (0, -0.25+bob, 0))
            wp = vec3_add(wp, vec3_scale(fwd, -w.recoil_amount*0.3))

            glTranslatef(*wp)
            glRotatef(-math.degrees(player.yaw)+180, 0, 1, 0)
            glRotatef(math.degrees(player.pitch), 1, 0, 0)

            if current_weapon == 0: # M4A1 AR
                glColor3f(0.25, 0.25, 0.25)
                # Receiver
                glPushMatrix(); glScalef(0.06, 0.1, 0.5); draw_cube_solid(); glPopMatrix()
                # Barrel
                glColor3f(0.15, 0.15, 0.15)
                glPushMatrix(); glTranslatef(0, 0, 0.35); glScalef(0.025, 0.025, 0.4); draw_cube_solid(); glPopMatrix()
                # Mag
                glColor3f(0.2, 0.2, 0.2)
                glPushMatrix(); glTranslatef(0, -0.12, 0.1); glRotatef(10, 1,0,0); glScalef(0.05, 0.2, 0.1); draw_cube_solid(); glPopMatrix()
                # Sight
                glPushMatrix(); glTranslatef(0, 0.06, -0.05); glScalef(0.04, 0.06, 0.08); draw_cube_solid(); glPopMatrix()
                # Stock
                glPushMatrix(); glTranslatef(0, -0.05, -0.3); glScalef(0.05, 0.12, 0.2); draw_cube_solid(); glPopMatrix()
            
            elif current_weapon == 1: # SPAS-12 Shotgun
                glColor3f(0.15, 0.15, 0.15)
                # Receiver
                glPushMatrix(); glScalef(0.07, 0.12, 0.45); draw_cube_solid(); glPopMatrix()
                # Barrel
                glPushMatrix(); glTranslatef(0, 0.04, 0.35); glScalef(0.03, 0.03, 0.4); draw_cube_solid(); glPopMatrix()
                # Pump tube
                glPushMatrix(); glTranslatef(0, -0.02, 0.35); glScalef(0.04, 0.04, 0.35); draw_cube_solid(); glPopMatrix()
                # Pump handle
                glColor3f(0.25, 0.25, 0.25)
                glPushMatrix(); glTranslatef(0, -0.03, 0.2); glScalef(0.06, 0.06, 0.2); draw_cube_solid(); glPopMatrix()
                # Folded Stock (top)
                glColor3f(0.2, 0.2, 0.2)
                glPushMatrix(); glTranslatef(0, 0.07, -0.1); glScalef(0.05, 0.02, 0.3); draw_cube_solid(); glPopMatrix()

            elif current_weapon == 2: # Barrett .50 Sniper
                glColor3f(0.35, 0.35, 0.32)
                # Huge Receiver
                glPushMatrix(); glScalef(0.09, 0.15, 0.6); draw_cube_solid(); glPopMatrix()
                # Massive Barrel
                glColor3f(0.1, 0.1, 0.1)
                glPushMatrix(); glTranslatef(0, 0.02, 0.5); glScalef(0.045, 0.045, 0.7); draw_cube_solid(); glPopMatrix()
                # Muzzle brake
                glColor3f(0.2, 0.2, 0.2)
                glPushMatrix(); glTranslatef(0, 0.02, 0.85); glScalef(0.07, 0.07, 0.1); draw_cube_solid(); glPopMatrix()
                # Big Scope
                glColor3f(0.1, 0.1, 0.1)
                glPushMatrix(); glTranslatef(0, 0.12, 0.05); glScalef(0.05, 0.05, 0.35); draw_cube_solid(); glPopMatrix()
                glPushMatrix(); glTranslatef(0, 0.12, -0.1); glScalef(0.06, 0.06, 0.05); draw_cube_solid(); glPopMatrix() # Eye piece
                # Mag
                glPushMatrix(); glTranslatef(0, -0.15, 0.2); glScalef(0.07, 0.2, 0.12); draw_cube_solid(); glPopMatrix()
                # Bipod folded loosely
                glPushMatrix(); glTranslatef(0.04, -0.08, 0.5); glRotatef(30, 0,0,1); glScalef(0.02, 0.2, 0.02); draw_cube_solid(); glPopMatrix()
                glPushMatrix(); glTranslatef(-0.04, -0.08, 0.5); glRotatef(-30, 0,0,1); glScalef(0.02, 0.2, 0.02); draw_cube_solid(); glPopMatrix()


            if current_weapon == 2:
                glPushMatrix(); glTranslatef(0,0.05,0)
                glScalef(0.035,0.06,0.12); glColor3f(0.12,0.12,0.12)
                draw_cube_solid(); glPopMatrix()
            glPopMatrix()

        # 2D HUD
        hud.draw(screen_w, screen_h, player, (w, current_weapon), enemies, is_aiming)

        pygame.display.flip()

    pygame.quit()

if __name__ == "__main__":
    main()
