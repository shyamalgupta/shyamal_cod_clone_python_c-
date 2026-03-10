"""
COD FPS — Game Event Handlers
These functions are called by the C++ engine when game events occur.
"""

# Track game statistics
stats = {
    "total_kills": 0,
    "killstreaks": [],
    "current_streak": 0,
    "damage_taken": 0.0,
}

def on_kill(kill_count):
    """Called when player kills an enemy."""
    stats["total_kills"] = kill_count
    stats["current_streak"] += 1
    
    streak = stats["current_streak"]
    if streak == 3:
        print("[EVENT] UAV READY! (3 kill streak)")
    elif streak == 5:
        print("[EVENT] PREDATOR MISSILE READY! (5 kill streak)")
    elif streak == 7:
        print("[EVENT] ATTACK HELICOPTER READY! (7 kill streak)")
    elif streak == 10:
        print("[EVENT] AC-130 READY! (10 kill streak)")
    elif streak == 15:
        print("[EVENT] TACTICAL NUKE READY! (15 kill streak)")
    elif streak == 25:
        print("[EVENT] NUCLEAR LAUNCH DETECTED! (25 kill streak)")
    else:
        print(f"[EVENT] Kill #{kill_count} (streak: {streak})")

def on_damage(damage, health_remaining):
    """Called when player takes damage."""
    stats["damage_taken"] += damage
    
    if health_remaining <= 25:
        print(f"[EVENT] CRITICAL HEALTH! ({health_remaining:.0f} HP remaining)")
    elif health_remaining <= 50:
        print(f"[EVENT] Low health warning ({health_remaining:.0f} HP)")

def on_death(kill_count, death_count):
    """Called when player dies."""
    stats["current_streak"] = 0
    kd = kill_count / max(death_count, 1)
    print(f"[EVENT] KILLED IN ACTION - K/D: {kd:.2f}")

def on_round_start():
    """Called at the start of a new round."""
    stats["current_streak"] = 0
    print("[EVENT] Round started! Good luck, soldier.")
