#!/usr/bin/env python3
"""
Run Multiple Subsystem Health Monitors
Launches multiple external system simulators, each monitoring a different component.

Supports two simulator types:
  - external_system.py  : Generic health monitor (all component types)
  - apcu_simulator.py   : Full APCU antenna system simulator with quadrants,
                           channels, temperature, board status, QTRMs, AQC

Works with any component type - the component_id just needs to match
the ID assigned on the canvas (e.g., component_1, component_2, etc.)
"""

import subprocess
import time
import signal
import sys
import argparse
import os

# ────────────────────────────────────────────────────────────────
#  Component type to simulator mapping
# ────────────────────────────────────────────────────────────────
COMPONENT_TYPE_MAP = {
    # component_id prefix -> (simulator script, component_type flag)
    "antenna": ("apcu_simulator.py", None),   # Use dedicated APCU simulator
    "power":   ("external_system.py", "PowerSystem"),
    "cooling": ("external_system.py", "LiquidCoolingUnit"),
    "comm":    ("external_system.py", "CommunicationSystem"),
    "computer":("external_system.py", "RadarComputer"),
}


class MultiSystemManager:
    def __init__(self, component_ids, host='localhost', port=12345, interval=2.0,
                 protocol='tcp', apcu_ids=None, type_map=None):
        self.component_ids = component_ids
        self.host = host
        self.port = port
        self.interval = interval
        self.protocol = protocol
        self.apcu_ids = set(apcu_ids or [])
        self.type_map = type_map or {}  # component_id -> component_type
        self.processes = []
    
    def _resolve_simulator(self, comp_id):
        """Determine which simulator script and flags to use."""
        # Explicit APCU flag
        if comp_id in self.apcu_ids:
            return ("apcu_simulator.py", None)
        
        # Explicit type mapping
        if comp_id in self.type_map:
            ctype = self.type_map[comp_id]
            return ("external_system.py", ctype)
        
        # Guess from component ID prefix
        lower_id = comp_id.lower()
        for prefix, (script, ctype) in COMPONENT_TYPE_MAP.items():
            if lower_id.startswith(prefix):
                return (script, ctype)
        
        # Default: generic external system with no type
        return ("external_system.py", None)
    
    def start(self):
        """Start all external systems"""
        print("=" * 65)
        print("  SUBSYSTEM HEALTH MONITOR MANAGER")
        print("=" * 65)
        print(f"  Components: {', '.join(self.component_ids)}")
        if self.apcu_ids:
            print(f"  APCU IDs  : {', '.join(self.apcu_ids)}")
        print(f"  Server    : {self.host}:{self.port}")
        print(f"  Protocol  : {self.protocol.upper()}")
        print(f"  Interval  : {self.interval}s base")
        print("=" * 65)
        print()
        
        script_dir = os.path.dirname(os.path.abspath(__file__))
        
        # Give slightly different intervals to each system to avoid synchronization
        for i, comp_id in enumerate(self.component_ids):
            comp_interval = self.interval + (i * 0.3)
            script, ctype = self._resolve_simulator(comp_id)
            script_path = os.path.join(script_dir, script)
            
            cmd = [
                sys.executable,
                script_path,
                comp_id,
                '--host', self.host,
                '--port', str(self.port),
                '--interval', str(comp_interval),
            ]
            
            # Add protocol for external_system.py (apcu_simulator always uses TCP)
            if script == "external_system.py":
                cmd.extend(['--protocol', self.protocol])
                if ctype:
                    cmd.extend(['--type', ctype])
            
            sim_label = "APCU" if script == "apcu_simulator.py" else (ctype or "generic")
            print(f"  Starting {sim_label} monitor for {comp_id} "
                  f"(interval: {comp_interval:.1f}s, script: {script})")
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

  # Use APCU simulator for specific antenna components
  python3 run_multiple_systems.py --components antenna_1 power_1 cooling_1 --apcu antenna_1

  # All components with APCU antenna
  python3 run_multiple_systems.py --apcu component_1

  # Monitor via UDP (non-APCU components only)
  python3 run_multiple_systems.py --protocol udp --port 12346

Simulator selection:
  - Components listed in --apcu use the APCU antenna simulator (apcu_simulator.py)
  - Components starting with 'antenna' auto-select APCU simulator
  - Components starting with 'power', 'cooling', 'comm', 'computer'
    auto-select the matching subsystem type
  - All other components use the generic external_system.py
        """,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('--components', nargs='+', 
                       default=['component_1', 'component_2', 'component_3', 'component_4', 'component_5'],
                       help='Component IDs to monitor (default: component_1 through component_5)')
    parser.add_argument('--apcu', nargs='*', default=[],
                       help='Component IDs that should use the APCU antenna simulator')
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
    
    manager = MultiSystemManager(
        args.components, args.host, args.port, args.interval,
        args.protocol, apcu_ids=args.apcu
    )
    manager.start()
    manager.wait()

if __name__ == '__main__':
    main()
