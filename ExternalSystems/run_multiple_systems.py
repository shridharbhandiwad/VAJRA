#!/usr/bin/env python3
"""
Run Multiple External Systems
Launches multiple external system simulators, each controlling a different component
"""

import subprocess
import time
import signal
import sys
import argparse

class MultiSystemManager:
    def __init__(self, component_ids, host='localhost', port=12345, interval=2.0):
        self.component_ids = component_ids
        self.host = host
        self.port = port
        self.interval = interval
        self.processes = []
    
    def start(self):
        """Start all external systems"""
        print("Starting External Systems Manager")
        print(f"Components: {', '.join(self.component_ids)}")
        print(f"Server: {self.host}:{self.port}")
        print(f"Update interval: {self.interval} seconds")
        print()
        
        # Give slightly different intervals to each system to avoid synchronization
        for i, comp_id in enumerate(self.component_ids):
            # Vary the interval slightly for each component
            comp_interval = self.interval + (i * 0.3)
            
            cmd = [
                sys.executable,
                'external_system.py',
                comp_id,
                '--host', self.host,
                '--port', str(self.port),
                '--interval', str(comp_interval)
            ]
            
            print(f"Launching external system for {comp_id} (interval: {comp_interval:.1f}s)")
            proc = subprocess.Popen(cmd)
            self.processes.append(proc)
            
            # Small delay between starting processes
            time.sleep(0.5)
        
        print()
        print("All external systems started!")
        print("Press Ctrl+C to stop all systems")
        print()
    
    def wait(self):
        """Wait for all processes"""
        try:
            while True:
                # Check if any process has died
                for i, proc in enumerate(self.processes):
                    if proc.poll() is not None:
                        print(f"Process {i} (PID {proc.pid}) has terminated")
                        self.processes.remove(proc)
                
                if not self.processes:
                    print("All processes have terminated")
                    break
                
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nShutting down all external systems...")
            self.stop()
    
    def stop(self):
        """Stop all external systems"""
        for proc in self.processes:
            try:
                proc.terminate()
                proc.wait(timeout=5)
            except:
                proc.kill()
        
        self.processes = []
        print("All external systems stopped")

def main():
    parser = argparse.ArgumentParser(description='Run Multiple External Systems')
    parser.add_argument('--components', nargs='+', 
                       default=['component_1', 'component_2', 'component_3'],
                       help='Component IDs to simulate (default: component_1 component_2 component_3)')
    parser.add_argument('--host', default='localhost', help='Server host (default: localhost)')
    parser.add_argument('--port', type=int, default=12345, help='Server port (default: 12345)')
    parser.add_argument('--interval', type=float, default=2.0, 
                       help='Base update interval in seconds (default: 2.0)')
    
    args = parser.parse_args()
    
    manager = MultiSystemManager(args.components, args.host, args.port, args.interval)
    manager.start()
    manager.wait()

if __name__ == '__main__':
    main()
