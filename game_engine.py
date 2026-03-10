"""
COD FPS Engine — Core 3D rendering and game systems using Pygame + OpenGL
"""
import pygame
import math
import random
import sys
import os
from OpenGL.GL import *
from OpenGL.GLU import *

# Import Python configs
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'scripts'))
try:
    from weapons import get_weapons
    from ai_behavior import get_ai_params
    from events import on_kill, on_damage
    PYTHON_SCRIPTS_LOADED = True
except ImportError:
    PYTHON_SCRIPTS_LOADED = False

# ============================================================
# Vector Math
# ============================================================
def vec3_add(a, b): return (a[0]+b[0], a[1]+b[1], a[2]+b[2])
def vec3_sub(a, b): return (a[0]-b[0], a[1]-b[1], a[2]-b[2])
def vec3_scale(a, s): return (a[0]*s, a[1]*s, a[2]*s)
def vec3_length(a): return math.sqrt(a[0]**2 + a[1]**2 + a[2]**2)
def vec3_dist(a, b): return vec3_length(vec3_sub(a, b))
def vec3_normalize(a):
    l = vec3_length(a)
    return (a[0]/l, a[1]/l, a[2]/l) if l > 0.001 else (0,0,0)
def vec3_dot(a, b): return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]
def vec3_cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])

# ============================================================
# AABB Collision
# ============================================================
class AABB:
    def __init__(self, pos, size):
        self.min = (pos[0]-size[0]/2, pos[1], pos[2]-size[2]/2)
        self.max = (pos[0]+size[0]/2, pos[1]+size[1], pos[2]+size[2]/2)

    def collides_sphere(self, center, radius):
        cx = max(self.min[0], min(center[0], self.max[0]))
        cy = max(self.min[1], min(center[1], self.max[1]))
        cz = max(self.min[2], min(center[2], self.max[2]))
        dist_sq = (cx-center[0])**2 + (cy-center[1])**2 + (cz-center[2])**2
        return dist_sq < radius**2

    def ray_intersect(self, origin, direction, max_dist=1000):
        tmin = -1e30; tmax = 1e30
        for i in range(3):
            if abs(direction[i]) < 1e-8:
                if origin[i] < self.min[i] or origin[i] > self.max[i]: return None
            else:
                t1 = (self.min[i] - origin[i]) / direction[i]
                t2 = (self.max[i] - origin[i]) / direction[i]
                if t1 > t2: t1, t2 = t2, t1
                tmin = max(tmin, t1); tmax = min(tmax, t2)
                if tmin > tmax: return None
        if tmin < 0: tmin = tmax
        if tmin < 0 or tmin > max_dist: return None
        return tmin

# ============================================================
# Weapon
# ============================================================
class Weapon:
    def __init__(self, name, damage, fire_rate, mag_size, reload_time, accuracy, recoil, wrange, pellets=1):
        self.name = name
        self.damage = damage
        self.fire_rate = fire_rate
        self.mag_size = mag_size
        self.reload_time = reload_time
        self.accuracy = accuracy
        self.recoil = recoil
        self.range = wrange
        self.pellets = pellets
        self.current_ammo = mag_size
        self.reserve_ammo = mag_size * 4
        self.fire_cooldown = 0
        self.reload_timer = 0
        self.is_reloading = False
        self.recoil_amount = 0

    def update(self, dt):
        if self.fire_cooldown > 0: self.fire_cooldown -= dt
        if self.recoil_amount > 0: self.recoil_amount *= 0.9
        if self.is_reloading:
            self.reload_timer -= dt
            if self.reload_timer <= 0:
                needed = self.mag_size - self.current_ammo
                avail = min(needed, self.reserve_ammo)
                self.current_ammo += avail
                self.reserve_ammo -= avail
                self.is_reloading = False

    def fire(self):
        if self.is_reloading or self.fire_cooldown > 0: return False
        if self.current_ammo <= 0:
            self.reload()
            return False
        self.current_ammo -= 1
        self.fire_cooldown = 1.0 / self.fire_rate
        self.recoil_amount = self.recoil
        return True

    def reload(self):
        if self.is_reloading or self.current_ammo >= self.mag_size or self.reserve_ammo <= 0: return
        self.is_reloading = True
        self.reload_timer = self.reload_time

# ============================================================
# Enemy
# ============================================================
class Enemy:
    PATROL, ALERT, ATTACK, DEAD = 0, 1, 2, 3

    def __init__(self, pos, params=None):
        self.pos = list(pos)
        self.spawn = list(pos)
        self.yaw = random.uniform(0, 6.28)
        self.health = 80; self.max_health = 80
        self.speed = 3.5; self.radius = 0.4; self.height = 1.8
        self.state = self.PATROL
        self.detect_range = 20; self.attack_range = 15
        self.accuracy = 0.15; self.reaction_time = 1.5
        self.reaction_timer = 0; self.fire_rate = 2.0
        self.fire_cooldown = 0; self.damage = 6
        self.patrol_target = self._rand_patrol()
        self.patrol_wait = 0; self.death_timer = 0
        self.respawn_time = 5; self.hit_flash = 0
        self.walk_time = random.uniform(0, 100) # For walking animation
        self.color = (0.3+random.uniform(-0.05,0.05), 0.35+random.uniform(-0.05,0.05), 0.3+random.uniform(-0.05,0.05)) # Tactical grey/green
        if params:
            self.detect_range = params.get('detection_range', 20)
            self.attack_range = params.get('attack_range', 15)
            self.accuracy = params.get('accuracy', 0.15)
            self.reaction_time = params.get('reaction_time', 1.5)
            self.speed = params.get('speed', 3.5)
            self.health = params.get('health', 80)
            self.max_health = self.health
            self.fire_rate = params.get('fire_rate', 1.2)
            self.damage = params.get('damage', 6)

    def _rand_patrol(self):
        return [self.spawn[0]+random.uniform(-20,20), 0, self.spawn[2]+random.uniform(-20,20)]

    def get_aabb(self):
        return AABB(self.pos, (0.7, self.height, 0.5))

    def update(self, player_pos, dt, colliders):
        if self.hit_flash > 0: self.hit_flash -= dt
        if self.state == self.DEAD:
            self.death_timer -= dt
            if self.death_timer <= 0:
                self.pos = list(self.spawn)
                self.health = self.max_health
                self.state = self.PATROL
                self.patrol_target = self._rand_patrol()
            return False
        
        self.fire_cooldown -= dt
        pdist = vec3_dist(self.pos, player_pos)
        is_moving = False

        if self.state == self.PATROL:
            to_t = vec3_sub(self.patrol_target, self.pos)
            d = math.sqrt(to_t[0]**2 + to_t[2]**2)
            if d < 1.5:
                self.patrol_wait -= dt
                if self.patrol_wait <= 0:
                    self.patrol_target = self._rand_patrol()
                    self.patrol_wait = random.uniform(1,3)
            else:
                dn = (to_t[0]/d, 0, to_t[2]/d)
                self.pos[0] += dn[0] * self.speed * 0.5 * dt
                self.pos[2] += dn[2] * self.speed * 0.5 * dt
                self.yaw = math.atan2(dn[0], dn[2])
                is_moving = True
            if pdist < self.detect_range:
                self.state = self.ALERT
                self.reaction_timer = self.reaction_time

        elif self.state == self.ALERT:
            to_p = vec3_sub(player_pos, self.pos)
            self.yaw = math.atan2(to_p[0], to_p[2])
            self.reaction_timer -= dt
            if self.reaction_timer <= 0:
                self.state = self.ATTACK if pdist < self.attack_range else self.PATROL

        elif self.state == self.ATTACK:
            to_p = vec3_sub(player_pos, self.pos)
            self.yaw = math.atan2(to_p[0], to_p[2])
            if pdist > self.attack_range * 0.7:
                dn = vec3_normalize(to_p)
                self.pos[0] += dn[0] * self.speed * dt
                self.pos[2] += dn[2] * self.speed * dt
                is_moving = True
            elif pdist < 5:
                dn = vec3_normalize(to_p)
                self.pos[0] -= dn[0] * self.speed * 0.6 * dt
                self.pos[2] -= dn[2] * self.speed * 0.6 * dt
                is_moving = True
            # Strafe
            strafe = math.sin(pygame.time.get_ticks()*0.002 + self.spawn[0])
            r = (math.cos(self.yaw), 0, -math.sin(self.yaw))
            self.pos[0] += r[0] * strafe * self.speed * 0.3 * dt
            self.pos[2] += r[2] * strafe * self.speed * 0.3 * dt
            if pdist > self.detect_range * 2: self.state = self.PATROL
            
            # Shoot at player
            if self.fire_cooldown <= 0 and pdist < self.attack_range:
                hit_chance = self.accuracy * (1.0 - pdist/(self.attack_range*2))
                self.fire_cooldown = 1.0 / self.fire_rate
                if random.random() < hit_chance:
                    return True  # Hit player
        
        # Update walk animation
        if is_moving:
            self.walk_time += dt * 15.0
        else:
            # Stand still smoothly
            self.walk_time += (0 - self.walk_time) * min(1.0, dt * 10)

        # Clamp to ground and boundaries
        self.pos[1] = 0
        self.pos[0] = max(-58, min(58, self.pos[0]))
        self.pos[2] = max(-58, min(58, self.pos[2]))
        return False

    def take_damage(self, dmg):
        self.health -= dmg
        self.hit_flash = 0.15
        if self.health <= 0:
            self.health = 0
            self.state = self.DEAD
            self.death_timer = self.respawn_time
            return True
        if self.state == self.PATROL: self.state = self.ATTACK
        return False

    def draw(self):
        if self.state == self.DEAD: return
        c = (1,0,0) if self.hit_flash > 0 else self.color
        
        glPushMatrix()
        glTranslatef(*self.pos)
        glRotatef(math.degrees(self.yaw), 0, 1, 0)
        
        # Calculate walk cycle
        leg_angle = math.sin(self.walk_time) * 35.0
        arm_angle = math.cos(self.walk_time) * 15.0
        
        # Lower Body (Legs)
        glColor3f(*c)
        glPushMatrix()
        glTranslatef(0.15, 0.45, 0) # Right leg
        glTranslatef(0, 0.45, 0); glRotatef(-leg_angle, 1, 0, 0); glTranslatef(0, -0.45, 0)
        glScalef(0.25, 0.9, 0.25)
        draw_cube_solid()
        glPopMatrix()
        
        glPushMatrix()
        glTranslatef(-0.15, 0.45, 0) # Left leg
        glTranslatef(0, 0.45, 0); glRotatef(leg_angle, 1, 0, 0); glTranslatef(0, -0.45, 0)
        glScalef(0.25, 0.9, 0.25)
        draw_cube_solid()
        glPopMatrix()

        # Torso (Tactical Vest)
        glColor3f(*c)
        glPushMatrix()
        glTranslatef(0, 1.25, 0)
        glScalef(0.55, 0.7, 0.35)
        draw_cube_solid()
        glPopMatrix()
        
        # Tactical Pouches
        glColor3f(0.2, 0.2, 0.2)
        glPushMatrix()
        glTranslatef(0, 1.15, 0.2)
        glScalef(0.4, 0.2, 0.1)
        draw_cube_solid()
        glPopMatrix()

        # Head & Helmet
        glPushMatrix()
        glTranslatef(0, 1.75, 0)
        # Face/Skin
        draw_sphere(0.18, (0.9, 0.75, 0.65))
        # Helmet
        glColor3f(0.2, 0.25, 0.2)
        glPushMatrix()
        glTranslatef(0, 0.05, 0)
        glScalef(0.23, 0.18, 0.23)
        draw_cube_solid()
        glPopMatrix()
        # Visor/Goggles
        glColor3f(0.1, 0.1, 0.1)
        glPushMatrix()
        glTranslatef(0, 0, 0.15)
        glScalef(0.2, 0.08, 0.1)
        draw_cube_solid()
        glPopMatrix()
        glPopMatrix()

        # Arms and Weapon
        # Right Arm (holding trigger)
        glColor3f(*c)
        glPushMatrix()
        glTranslatef(0.35, 1.4, 0)
        glRotatef( arm_angle, 1,0,0 )
        glRotatef( -75, 1,0,0 ) # Raise arm to hold gun
        glTranslatef(0, -0.3, 0)
        glScalef(0.18, 0.6, 0.18)
        draw_cube_solid()
        glPopMatrix()
        
        # Left Arm (holding barrel)
        glPushMatrix()
        glTranslatef(-0.35, 1.4, 0)
        glRotatef( -arm_angle, 1,0,0 )
        glRotatef( -60, 1,0,0 )
        glRotatef( 30, 0,1,0 )
        glTranslatef(0, -0.3, 0)
        glScalef(0.18, 0.6, 0.18)
        draw_cube_solid()
        glPopMatrix()

        # Detailed Assault Rifle
        glPushMatrix()
        glTranslatef(0.15, 1.25, 0.3) # Held by right hand
        
        # Receiver
        glColor3f(0.15, 0.15, 0.15)
        glPushMatrix(); glScalef(0.08, 0.12, 0.4); draw_cube_solid(); glPopMatrix()
        # Barrel
        glColor3f(0.1, 0.1, 0.1)
        glPushMatrix(); glTranslatef(0, 0.02, 0.3); glScalef(0.04, 0.04, 0.3); draw_cube_solid(); glPopMatrix()
        # Magazine
        glColor3f(0.12, 0.12, 0.12)
        glPushMatrix(); glTranslatef(0, -0.15, 0.1); glRotatef(15, 1,0,0); glScalef(0.06, 0.2, 0.1); draw_cube_solid(); glPopMatrix()
        # Stock
        glPushMatrix(); glTranslatef(0, -0.05, -0.25); glScalef(0.06, 0.15, 0.2); draw_cube_solid(); glPopMatrix()
        
        glPopMatrix()
        glPopMatrix()

# ============================================================
# GL Drawing Helpers — Enhanced with normals & textures
# ============================================================
_cube_faces = [
    # (normal, vertices, tex_coords)
    ((0,0,1),  ((-.5,-.5,.5),(.5,-.5,.5),(.5,.5,.5),(-.5,.5,.5)), ((0,0),(1,0),(1,1),(0,1))),
    ((0,0,-1), ((-.5,-.5,-.5),(-.5,.5,-.5),(.5,.5,-.5),(.5,-.5,-.5)), ((1,0),(1,1),(0,1),(0,0))),
    ((0,1,0),  ((-.5,.5,-.5),(-.5,.5,.5),(.5,.5,.5),(.5,.5,-.5)), ((0,1),(0,0),(1,0),(1,1))),
    ((0,-1,0), ((-.5,-.5,-.5),(.5,-.5,-.5),(.5,-.5,.5),(-.5,-.5,.5)), ((0,1),(1,1),(1,0),(0,0))),
    ((1,0,0),  ((.5,-.5,-.5),(.5,.5,-.5),(.5,.5,.5),(.5,-.5,.5)), ((1,0),(1,1),(0,1),(0,0))),
    ((-1,0,0), ((-.5,-.5,-.5),(-.5,-.5,.5),(-.5,.5,.5),(-.5,.5,-.5)), ((0,0),(1,0),(1,1),(0,1))),
]

def draw_cube_solid(tex=None):
    if tex:
        glEnable(GL_TEXTURE_2D)
        glBindTexture(GL_TEXTURE_2D, tex)
    else: glDisable(GL_TEXTURE_2D)

    glBegin(GL_QUADS)
    for normal, verts, uvs in _cube_faces:
        glNormal3f(*normal)
        for v, uv in zip(verts, uvs):
            if tex: glTexCoord2f(uv[0], uv[1])
            glVertex3f(*v)
    glEnd()
    glDisable(GL_TEXTURE_2D)

def draw_cube_textured(pos, size, color, shade_sides=True, tex=None):
    """Draw a cube with per-face shading and optional texture."""
    glPushMatrix()
    glTranslatef(pos[0], pos[1]+size[1]/2, pos[2])
    glScalef(size[0], size[1], size[2])
    # Draw each face with slight color variation for 3D look
    if tex:
        glEnable(GL_TEXTURE_2D)
        glBindTexture(GL_TEXTURE_2D, tex)
        glColor3f(1,1,1) # White to show texture
    else:
        glDisable(GL_TEXTURE_2D)
        glColor3f(color[0], color[1], color[2])

    face_shades = [1.0, 0.7, 1.1, 0.6, 0.85, 0.85] if shade_sides else [1]*6
    glBegin(GL_QUADS)
    for i, (normal, verts, uvs) in enumerate(_cube_faces):
        s = face_shades[i]
        if not tex:
            glColor3f(min(color[0]*s,1), min(color[1]*s,1), min(color[2]*s,1))
        glNormal3f(*normal)
        for v, uv in zip(verts, uvs):
            if tex: glTexCoord2f(uv[0]*size[0], uv[1]*size[1]) # Tiling
            glVertex3f(*v)
    glEnd()
    glDisable(GL_TEXTURE_2D)
    # Subtle dark edges
    glColor4f(0, 0, 0, 0.15)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
    draw_cube_solid()
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)
    glPopMatrix()

def draw_cube(pos, size, color, tex=None):
    draw_cube_textured(pos, size, color, shade_sides=True, tex=tex)

def draw_sphere(radius, color):
    glColor3f(*color)
    quad = gluNewQuadric()
    gluQuadricNormals(quad, GLU_SMOOTH)
    gluSphere(quad, radius, 12, 12)
    gluDeleteQuadric(quad)

def draw_ground(size, color, tex=None):
    hs = size / 2
    if tex:
        glEnable(GL_TEXTURE_2D)
        glBindTexture(GL_TEXTURE_2D, tex)
        glColor3f(1, 1, 1)
    else: glDisable(GL_TEXTURE_2D)

    # Multi-tile ground for texture-like appearance
    tile = 5.0
    for x in range(int(-hs), int(hs), int(tile)):
        for z in range(int(-hs), int(hs), int(tile)):
            if not tex:
                # Checkerboard pattern for visual texture (fallback)
                checker = ((x // int(tile)) + (z // int(tile))) % 2
                shade = 1.0 if checker == 0 else 0.92
                glColor3f(color[0]*shade, color[1]*shade, color[2]*shade)
            glNormal3f(0, 1, 0)
            glBegin(GL_QUADS)
            if tex: glTexCoord2f(0, 0)
            glVertex3f(x, 0, z)
            if tex: glTexCoord2f(1, 0)
            glVertex3f(x+tile, 0, z)
            if tex: glTexCoord2f(1, 1)
            glVertex3f(x+tile, 0, z+tile)
            if tex: glTexCoord2f(0, 1)
            glVertex3f(x, 0, z+tile)
            glEnd()
    if tex: glDisable(GL_TEXTURE_2D)
    # Grid lines
    glColor4f(0.3, 0.35, 0.25, 0.2)
    glBegin(GL_LINES)
    for i in range(int(-hs), int(hs)+1, 10):
        glVertex3f(i, 0.02, -hs); glVertex3f(i, 0.02, hs)
        glVertex3f(-hs, 0.02, i); glVertex3f(hs, 0.02, i)
    glEnd()

# Texture Loading & Generation Helper
def generate_texture(tex_type, size=256):
    import pygame
    import numpy as np
    
    # Generate base noise
    noise = np.random.randint(0, 50, (size, size, 3), dtype=np.uint8)
    
    if tex_type == 'grass':
        base = np.array([40, 80, 30], dtype=np.uint8)
        img = np.clip(base + noise - 25, 0, 255)
    elif tex_type == 'wall':
        base = np.array([120, 110, 100], dtype=np.uint8)
        img = np.clip(base + noise - 25, 0, 255)
        # Add basic brick lines
        for y in range(0, size, 32):
            img[y:y+2, :] = [80, 75, 70]
            offset = 32 if (y // 32) % 2 == 0 else 0
            for x in range(offset, size, 64):
                img[y:y+32, x:x+2] = [80, 75, 70]
    elif tex_type == 'wood':
        base = np.array([160, 110, 70], dtype=np.uint8)
        img = np.clip(base + noise - 25, 0, 255)
        # Add wood grain striping
        for y in range(0, size, 16):
            img[y:y+np.random.randint(2,5), :] -= np.random.randint(20, 40)
        # Add crate borders
        img[0:8, :] = [80, 50, 30]
        img[-8:, :] = [80, 50, 30]
        img[:, 0:8] = [80, 50, 30]
        img[:, -8:] = [80, 50, 30]
        img = np.clip(img, 0, 255)
    else: # concrete
        base = np.array([100, 105, 110], dtype=np.uint8)
        img = np.clip(base + noise - 25, 0, 255)

    # Add alpha channel
    alpha = np.full((size, size, 1), 255, dtype=np.uint8)
    img_rgba = np.concatenate((img, alpha), axis=2)
    
    data = img_rgba.tobytes()
    tex = glGenTextures(1)
    glBindTexture(GL_TEXTURE_2D, tex)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)
    glGenerateMipmap(GL_TEXTURE_2D)
    return tex

def draw_tree(pos):
    """Draw a simple stylized pine tree."""
    x, y, z = pos
    # Trunk
    draw_cube_solid(tex=None) # Disable texture temporarily
    draw_cube((x, y, z), (0.6, 3.0, 0.6), (0.35, 0.25, 0.15))
    # Leaves (layered)
    draw_cube((x, y+2.0, z), (3.0, 1.5, 3.0), (0.1, 0.4, 0.1))
    draw_cube((x, y+3.2, z), (2.0, 1.5, 2.0), (0.15, 0.45, 0.15))
    draw_cube((x, y+4.4, z), (1.0, 1.5, 1.0), (0.2, 0.5, 0.2))

# ============================================================
# Particle System
# ============================================================
class Particle:
    __slots__ = ['pos', 'vel', 'color', 'life', 'max_life', 'size', 'active']
    def __init__(self):
        self.active = False

class ParticleSystem:
    def __init__(self, max_p=1500):
        self.particles = [Particle() for _ in range(max_p)]

    def _get_free(self):
        for p in self.particles:
            if not p.active: return p
        return None

    def update(self, dt):
        for p in self.particles:
            if not p.active: continue
            p.pos = vec3_add(p.pos, vec3_scale(p.vel, dt))
            p.vel = (p.vel[0], p.vel[1] - 5*dt, p.vel[2])
            p.life -= dt
            if p.life <= 0: p.active = False

    def draw(self):
        for p in self.particles:
            if not p.active: continue
            alpha = p.life / p.max_life
            s = p.size * (0.5 + 0.5*alpha)
            glPushMatrix()
            glTranslatef(*p.pos)
            glScalef(s, s, s)
            glColor4f(p.color[0], p.color[1], p.color[2], alpha)
            draw_cube_solid()
            glPopMatrix()

    def spawn_muzzle(self, pos, direction):
        for i in range(8):
            p = self._get_free()
            if not p: return
            p.active = True; p.pos = pos
            p.vel = (direction[0]*random.uniform(5,15)+random.uniform(-3,3),
                     direction[1]*random.uniform(5,15)+random.uniform(-1,3),
                     direction[2]*random.uniform(5,15)+random.uniform(-3,3))
            b = random.uniform(0.7, 1.0)
            p.color = (1.0, b, 0.2) if i < 3 else (1.0, 0.7*b, 0.1)
            p.life = random.uniform(0.03, 0.08); p.max_life = p.life
            p.size = random.uniform(0.05, 0.12)

    def spawn_blood(self, pos):
        for _ in range(10):
            p = self._get_free()
            if not p: return
            p.active = True; p.pos = pos
            p.vel = (random.uniform(-4,4), random.uniform(0,6), random.uniform(-4,4))
            p.color = (random.uniform(0.6,0.85), random.uniform(0,0.1), random.uniform(0,0.08))
            p.life = random.uniform(0.3, 0.7); p.max_life = p.life
            p.size = random.uniform(0.04, 0.1)

    def spawn_impact(self, pos):
        for _ in range(6):
            p = self._get_free()
            if not p: return
            p.active = True; p.pos = pos
            p.vel = (random.uniform(-5,5), random.uniform(1,8), random.uniform(-5,5))
            p.color = (0.7, 0.67, 0.6)
            p.life = random.uniform(0.2, 0.4); p.max_life = p.life
            p.size = random.uniform(0.03, 0.07)
