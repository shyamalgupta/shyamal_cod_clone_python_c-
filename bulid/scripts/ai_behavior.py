"""
COD FPS — AI Behavior Configuration
Tweak these parameters to make enemies harder or easier.
*** NERFED — enemies are now easier for a more fun experience ***
"""

def get_ai_params():
    return {
        "detection_range": 20.0,    # Reduced from 30 — enemies spot you later
        "attack_range": 15.0,       # Reduced from 25 — enemies engage closer
        "accuracy": 0.15,           # Reduced from 0.35 — enemies miss more
        "reaction_time": 1.5,       # Increased from 0.8 — slower reactions
        "patrol_speed": 1.5,        # Slower patrol
        "attack_speed": 2.5,        # Slower attack movement
        "fire_rate": 1.2,           # Reduced from 2.0 — shoot less often
        "damage": 6.0,              # Halved from 12 — much less damage per hit
        "aggression": 0.4,          # Less aggressive pushes
    }

# Difficulty presets
DIFFICULTY_PRESETS = {
    "easy": {
        "detection_range": 15.0,
        "attack_range": 10.0,
        "accuracy": 0.10,
        "reaction_time": 2.0,
        "damage": 4.0,
    },
    "normal": {
        "detection_range": 20.0,
        "attack_range": 15.0,
        "accuracy": 0.15,
        "reaction_time": 1.5,
        "damage": 6.0,
    },
    "hard": {
        "detection_range": 35.0,
        "attack_range": 25.0,
        "accuracy": 0.40,
        "reaction_time": 0.6,
        "damage": 15.0,
    },
    "veteran": {
        "detection_range": 50.0,
        "attack_range": 40.0,
        "accuracy": 0.65,
        "reaction_time": 0.3,
        "damage": 22.0,
    },
}
