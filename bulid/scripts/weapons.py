"""
COD FPS — Weapon Configuration
Modify these values to tweak weapon balance.
Changes take effect on game restart.
"""

def get_weapons():
    return [
        {
            "name": "M4A1",
            "damage": 28.0,
            "fire_rate": 10.0,     # shots per second
            "mag_size": 30,
            "reload_time": 2.2,    # seconds
            "accuracy": 0.92,      # 0-1
            "recoil": 0.03,
            "range": 100.0,
            "pellets": 1,
        },
        {
            "name": "SPAS-12",
            "damage": 18.0,
            "fire_rate": 1.5,
            "mag_size": 8,
            "reload_time": 3.0,
            "accuracy": 0.70,
            "recoil": 0.12,
            "range": 25.0,
            "pellets": 8,          # shotgun spread
        },
        {
            "name": "BARRETT .50",
            "damage": 95.0,
            "fire_rate": 0.8,
            "mag_size": 5,
            "reload_time": 3.5,
            "accuracy": 0.98,
            "recoil": 0.20,
            "range": 200.0,
            "pellets": 1,
        },
    ]
