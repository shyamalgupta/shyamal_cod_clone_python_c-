"""
COD FPS — Arena Level Configuration
Defines the arena layout, spawns, and environment settings.
"""

# Number of AI enemies
enemy_count = 10

# Player spawn position
player_spawn = {"x": 0.0, "y": 0.0, "z": 0.0}

# Ground settings
ground_size = 120.0
ground_color = {"r": 50, "g": 58, "b": 44}
sky_color = {"r": 100, "g": 140, "b": 180}

# Building positions (x, y, z, width, height, depth)
buildings = [
    {"name": "Central Warehouse", "x": 0, "z": 25, "w": 12, "h": 6, "d": 8},
    {"name": "SW Barracks",       "x": -30, "z": -20, "w": 10, "h": 5, "d": 10},
    {"name": "NE Tower",          "x": 35, "z": 30, "w": 6, "h": 10, "d": 6},
    {"name": "SE Barracks",       "x": 30, "z": -25, "w": 14, "h": 4, "d": 6},
    {"name": "NW Bunker",         "x": -35, "z": 35, "w": 8, "h": 3, "d": 8},
    {"name": "East Garage",       "x": 45, "z": 5, "w": 10, "h": 4, "d": 8},
    {"name": "West HQ",           "x": -40, "z": 10, "w": 12, "h": 6, "d": 10},
    {"name": "South Outpost",     "x": 10, "z": -40, "w": 8, "h": 5, "d": 8},
]

# Enemy spawn points
enemy_spawns = [
    {"x": -25, "z": -15},
    {"x": 25, "z": -20},
    {"x": 30, "z": 25},
    {"x": -30, "z": 30},
    {"x": 40, "z": 0},
    {"x": -40, "z": 5},
    {"x": 5, "z": -35},
    {"x": -10, "z": 40},
    {"x": 20, "z": 15},
    {"x": -20, "z": -30},
]
