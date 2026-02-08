#!/usr/bin/env python3
"""
External System Simulator
Sends periodic messages to the Runtime Application to update component appearance
"""

import socket
import json
import time
import random
import argparse
import sys

class ExternalSystem:
    def __init__(self, component_id, host='localhost', port=12345):
        self.component_id = component_id
        self.host = host
        self.port = port
        self.socket = None
        self.connected = False
        
        # Predefined color palette
        self.colors = [
            '#FF0000',  # Red
            '#00FF00',  # Green
            '#0000FF',  # Blue
            '#FFFF00',  # Yellow
            '#FF00FF',  # Magenta
            '#00FFFF',  # Cyan
            '#FFA500',  # Orange
            '#800080',  # Purple
            '#FFC0CB',  # Pink
            '#A52A2A',  # Brown
        ]
        
    def connect(self):
        """Connect to the Runtime Application server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.connected = True
            print(f"[{self.component_id}] Connected to {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"[{self.component_id}] Connection failed: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from server"""
        if self.socket:
            self.socket.close()
            self.connected = False
            print(f"[{self.component_id}] Disconnected")
    
    def send_update(self, color, size):
        """Send an update message to the server"""
        if not self.connected:
            return False
        
        message = {
            "component_id": self.component_id,
            "color": color,
            "size": size
        }
        
        try:
            json_data = json.dumps(message) + '\n'
            self.socket.sendall(json_data.encode('utf-8'))
            print(f"[{self.component_id}] Sent: color={color}, size={size}")
            return True
        except Exception as e:
            print(f"[{self.component_id}] Send failed: {e}")
            self.connected = False
            return False
    
    def run(self, interval=2.0, size_min=30, size_max=100):
        """Run the external system, sending periodic updates"""
        print(f"[{self.component_id}] Starting external system simulator")
        print(f"[{self.component_id}] Update interval: {interval} seconds")
        print(f"[{self.component_id}] Size range: {size_min}-{size_max}")
        print()
        
        if not self.connect():
            return
        
        try:
            while True:
                # Generate random color and size
                color = random.choice(self.colors)
                size = random.uniform(size_min, size_max)
                
                # Send update
                if not self.send_update(color, size):
                    print(f"[{self.component_id}] Reconnecting...")
                    time.sleep(1)
                    if not self.connect():
                        break
                
                # Wait for next update
                time.sleep(interval)
                
        except KeyboardInterrupt:
            print(f"\n[{self.component_id}] Shutting down...")
        finally:
            self.disconnect()

def main():
    parser = argparse.ArgumentParser(description='External System Simulator')
    parser.add_argument('component_id', help='Component ID to update (e.g., component_1)')
    parser.add_argument('--host', default='localhost', help='Server host (default: localhost)')
    parser.add_argument('--port', type=int, default=12345, help='Server port (default: 12345)')
    parser.add_argument('--interval', type=float, default=2.0, 
                       help='Update interval in seconds (default: 2.0)')
    parser.add_argument('--size-min', type=float, default=30, 
                       help='Minimum component size (default: 30)')
    parser.add_argument('--size-max', type=float, default=100, 
                       help='Maximum component size (default: 100)')
    
    args = parser.parse_args()
    
    system = ExternalSystem(args.component_id, args.host, args.port)
    system.run(args.interval, args.size_min, args.size_max)

if __name__ == '__main__':
    main()
