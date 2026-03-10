import socket
import threading
import json
import time

HOST = '0.0.0.0'
PORT = 5555

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1) # Ultra-low latency

try:
    server.bind((HOST, PORT))
    server.listen()
    print(f"[*] Server started on {HOST}:{PORT}")
    print("[*] Waiting for clients...")
except socket.error as e:
    print(str(e))
    exit(1)

clients = {} # id -> conn
player_data = {} # id -> data dict
events_queue = [] # Queue for shots, kills, etc
id_counter = 0

def threaded_client(conn, player_id):
    global player_data
    
    # Send initial connection success with assigned ID
    conn.send(str.encode(str(player_id) + "\n"))
    
    # Initialize their state
    player_data[player_id] = {
        "id": player_id,
        "x": 0, "y": 0, "z": 0,
        "yaw": 0, "pitch": 0,
        "anim_time": 0,
        "weapon": 0,
        "health": 100,
        "is_dead": False
    }
    
    while True:
        try:
            data = conn.recv(2048).decode('utf-8')
            if not data:
                print(f"[-] Disconnected: Player {player_id}")
                break
                
            # Process incoming packets (split by newline since multiple might arrive)
            packets = data.strip().split('\n')
            for packet in packets:
                if not packet: continue
                msg = json.loads(packet)
                
                if msg["type"] == "update":
                    # Update this player's state
                    for k, v in msg["data"].items():
                        player_data[player_id][k] = v
                elif msg["type"] == "shoot":
                    events_queue.append({"type": "shoot", "shooter": player_id})
                elif msg["type"] == "hit":
                    # Client-Authoritative Hit Detection
                    target_id = msg["target"]
                    damage = msg["damage"]
                    if target_id in player_data and not player_data[target_id]["is_dead"]:
                        player_data[target_id]["health"] -= damage
                        if player_data[target_id]["health"] <= 0:
                            player_data[target_id]["is_dead"] = True
                        events_queue.append({"type": "hit", "shooter": player_id, "target": target_id, "damage": damage, "fatal": player_data[target_id]["is_dead"]})
                        
        except Exception as e:
            print(f"[-] Error from Player {player_id}: {e}")
            break

    print(f"[-] Lost connection to Player {player_id}")
    if player_id in clients: del clients[player_id]
    if player_id in player_data: del player_data[player_id]
    conn.close()

def game_loop():
    """ Runs at 60Hz and broadcasts all player states and events to everyone """
    global events_queue
    while True:
        time.sleep(1/60.0) # 60 Tickrate server
        state = {
            "players": player_data,
            "events": events_queue
        }
        state_json = json.dumps(state) + "\n"
        encoded = str.encode(state_json)
        
        # Broadcast to all
        dead_clients = []
        for pid, conn in clients.items():
            try:
                conn.sendall(encoded)
            except:
                dead_clients.append(pid)
                
        for pid in dead_clients:
            del clients[pid]
            if pid in player_data: del player_data[pid]
            
        events_queue.clear()

# Start background ticker
threading.Thread(target=game_loop, daemon=True).start()

# Accept connections
while True:
    conn, addr = server.accept()
    conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    print(f"[+] Connected to: {addr[0]}:{addr[1]}")
    
    id_counter += 1
    clients[id_counter] = conn
    
    t = threading.Thread(target=threaded_client, args=(conn, id_counter))
    t.daemon = True
    t.start()
