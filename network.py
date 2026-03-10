import socket
import threading
import json
import time

class Network:
    def __init__(self, server_ip='127.0.0.1', port=5555):
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.server = server_ip
        self.port = port
        self.addr = (self.server, self.port)
        self.id = self.connect()
        
        self.latest_state = {"players": {}, "events": []}
        self.running = True
        
        if self.id is not None:
            # Start background thread to seamlessly receive states
            threading.Thread(target=self.receive_loop, daemon=True).start()

    def connect(self):
        try:
            self.client.connect(self.addr)
            return int(self.client.recv(2048).decode().strip())
        except:
            print("[NETWORK ERROR] Failed to connect to server.")
            return None

    def send_update(self, data):
        """ Send my local position/rotation updating to server """
        if self.id is None: return
        try:
            msg = {"type": "update", "data": data}
            self.client.sendall(str.encode(json.dumps(msg) + "\n"))
        except socket.error as e:
            print(e)
            
    def send_hit(self, target_id, damage):
        """ Client authoritative hit """
        if self.id is None: return
        try:
            msg = {"type": "hit", "target": target_id, "damage": damage}
            self.client.sendall(str.encode(json.dumps(msg) + "\n"))
        except socket.error as e:
            print(e)
            
    def send_shoot(self):
        if self.id is None: return
        try:
            msg = {"type": "shoot"}
            self.client.sendall(str.encode(json.dumps(msg) + "\n"))
        except socket.error as e:
            print(e)

    def receive_loop(self):
        """ Runs in background thread to constantly pull game state without lagging framerate """
        buffer = ""
        while self.running:
            try:
                data = self.client.recv(8192).decode('utf-8')
                if not data:
                    break
                buffer += data
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    if line:
                        state = json.loads(line)
                        self.latest_state = state
            except:
                break
        self.running = False
