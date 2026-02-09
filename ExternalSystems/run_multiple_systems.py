#!/usr/bin/env python3
"""
Run Multiple Subsystem Health Monitors
Launches multiple external system simulators, each monitoring a different component.

Works with any component type - the component_id just needs to match
the ID assigned on the canvas (e.g., component_1, component_2, etc.)
"""

import subprocess
import time
import signal
import sys
import argparse

class MultiSystemManager:
    def __init__(self, component_ids, host='localhost', port=12345, interval=2.0, protocol='tcp'):
        self.component_ids = component_ids
        self.host = host
        self.port = port
        self.interval = interval
        self.protocol = protocol
        self.processes = []
    
    def start(self):
        """Start all external systems"""
        print("=" * 60)
        print("  SUBSYSTEM HEALTH MONITOR MANAGER")
        print("=" * 60)
        print(f"  Components: {', '.join(self.component_ids)}")
        print(f"  Server:     {self.host}:{self.port}")
        print(f"  Protocol:   {self.protocol.upper()}")
        print(f"  Interval:   {self.interval}s base")
        print("=" * 60)
        print()
        
        # Give slightly different intervals to each system to avoid synchronization
        for i, comp_id in enumerate(self.component_ids):
            comp_interval = self.interval + (i * 0.3)
            
            cmd = [
                sys.executable,
                'external_system.py',
                comp_id,
                '--host', self.host,
                '--port', str(self.port),
                '--protocol', self.protocol,
                '--interval', str(comp_interval)
            ]
            
            print(f"  Starting monitor for {comp_id} (interval: {comp_interval:.1f}s)")
            proc = subprocess.Popen(cmd)
            self.processes.append(proc)
            
            # Small delay between starting processes
            time.sleep(0.5)
        
        print()
        print("All health monitors started!")
        print("Press Ctrl+C to stop all monitors")
        print()
    
    def wait(self):
        """Wait for all processes"""
        try:
            while True:
                for i, proc in enumerate(self.processes):
                    if proc.poll() is not None:
                        print(f"Process {i} (PID {proc.pid}) has terminated")
                        self.processes.remove(proc)
                
                if not self.processes:
                    print("All processes have terminated")
                    break
                
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nShutting down all health monitors...")
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
        print("All health monitors stopped")

def main():
    parser = argparse.ArgumentParser(
        description='Run Multiple Subsystem Health Monitors',
        epilog="""
Examples:
  # Monitor default components
  python3 run_multiple_systems.py

  # Monitor specific components (use canvas IDs like component_1)
  python3 run_multiple_systems.py --components component_1 component_2 component_3

  # Monitor via UDP
  python3 run_multiple_systems.py --protocol udp --port 12346
        """,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('--components', nargs='+', 
                       default=['component_1', 'component_2', 'component_3', 'component_4', 'component_5'],
                       help='Component IDs to monitor (default: component_1 through component_5)')
    parser.add_argument('--host', default='localhost', 
                       help='Server host (default: localhost)')
    parser.add_argument('--port', type=int, default=None,
                       help='Server port (default: 12345 for TCP, 12346 for UDP)')
    parser.add_argument('--protocol', choices=['tcp', 'udp'], default='tcp',
                       help='Communication protocol (default: tcp)')
    parser.add_argument('--interval', type=float, default=2.0, 
                       help='Base health update interval in seconds (default: 2.0)')
    
    args = parser.parse_args()
    
    if args.port is None:
        args.port = 12346 if args.protocol == 'udp' else 12345
    
    manager = MultiSystemManager(args.components, args.host, args.port, args.interval, args.protocol)
    manager.start()
    manager.wait()

if __name__ == '__main__':
    main()
