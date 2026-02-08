#!/usr/bin/env python3
"""
External Radar Subsystem Simulator
Sends periodic health status messages to the Runtime Application
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
        
        # Health status color mapping
        self.health_colors = {
            'operational': '#00FF00',    # Green - Normal operation
            'warning': '#FFFF00',        # Yellow - Minor issues
            'degraded': '#FFA500',       # Orange - Performance degraded
            'critical': '#FF0000',       # Red - Critical issues
            'offline': '#808080',        # Gray - System offline
        }
        
        # Current health state (starts healthy)
        self.current_health = 95.0
        self.current_status = 'operational'
        
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
    
    def send_update(self, status, health):
        """Send a health status update message to the server"""
        if not self.connected:
            return False
        
        color = self.health_colors.get(status, '#808080')
        
        message = {
            "component_id": self.component_id,
            "color": color,
            "size": health
        }
        
        try:
            json_data = json.dumps(message) + '\n'
            self.socket.sendall(json_data.encode('utf-8'))
            print(f"[{self.component_id}] Health Update: status={status}, health={health:.1f}%, color={color}")
            return True
        except Exception as e:
            print(f"[{self.component_id}] Send failed: {e}")
            self.connected = False
            return False
    
    def simulate_health_change(self):
        """Simulate realistic health changes for a radar subsystem"""
        # Randomly adjust health by a small amount
        health_change = random.uniform(-5.0, 3.0)
        self.current_health += health_change
        
        # Keep health in valid range
        self.current_health = max(0.0, min(100.0, self.current_health))
        
        # Determine status based on health level
        if self.current_health >= 90:
            self.current_status = 'operational'
        elif self.current_health >= 70:
            self.current_status = 'warning'
        elif self.current_health >= 40:
            self.current_status = 'degraded'
        elif self.current_health >= 10:
            self.current_status = 'critical'
        else:
            self.current_status = 'offline'
        
        # Occasionally introduce a random event (10% chance)
        if random.random() < 0.1:
            event_type = random.choice(['spike', 'drop', 'restore'])
            if event_type == 'spike':
                self.current_health = min(100.0, self.current_health + random.uniform(10, 20))
                print(f"[{self.component_id}] EVENT: Health spike!")
            elif event_type == 'drop':
                self.current_health = max(0.0, self.current_health - random.uniform(15, 30))
                print(f"[{self.component_id}] EVENT: Health drop!")
            elif event_type == 'restore':
                self.current_health = random.uniform(85, 100)
                print(f"[{self.component_id}] EVENT: System restored!")
        
        return self.current_status, self.current_health
    
    def run(self, interval=2.0):
        """Run the external system, sending periodic health updates"""
        print(f"[{self.component_id}] Starting Radar Subsystem Health Monitor")
        print(f"[{self.component_id}] Update interval: {interval} seconds")
        print(f"[{self.component_id}] Initial health: {self.current_health:.1f}%")
        print()
        
        if not self.connect():
            return
        
        try:
            while True:
                # Simulate health changes
                status, health = self.simulate_health_change()
                
                # Send health update
                if not self.send_update(status, health):
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
    parser = argparse.ArgumentParser(description='External Radar Subsystem Health Monitor')
    parser.add_argument('component_id', help='Subsystem ID to monitor (e.g., antenna_1, power_1)')
    parser.add_argument('--host', default='localhost', help='Server host (default: localhost)')
    parser.add_argument('--port', type=int, default=12345, help='Server port (default: 12345)')
    parser.add_argument('--interval', type=float, default=2.0, 
                       help='Health update interval in seconds (default: 2.0)')
    
    args = parser.parse_args()
    
    system = ExternalSystem(args.component_id, args.host, args.port)
    system.run(args.interval)

if __name__ == '__main__':
    main()
