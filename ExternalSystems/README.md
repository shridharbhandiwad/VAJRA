# External Radar Subsystem Health Monitors

Python scripts that simulate external radar subsystems sending periodic health status updates to the Runtime Application.

## Overview

These health monitors simulate real-world radar subsystems that report their operational status periodically. Each monitor:
- Connects to the Runtime Application via TCP (port 12345)
- Sends JSON health update messages
- Simulates realistic health degradation and recovery
- Generates random events (failures, recoveries)

## Health Status Simulation

### Health Levels and Status Mapping

| Health % | Status | Color | Description |
|----------|--------|-------|-------------|
| 90-100 | Operational | Green | Normal operation |
| 70-89 | Warning | Yellow | Minor issues |
| 40-69 | Degraded | Orange | Performance issues |
| 10-39 | Critical | Red | Serious problems |
| 0-9 | Offline | Gray | System down |

### Simulation Behavior

**Gradual Changes:**
- Health changes by ±5% per update
- Simulates normal wear and tear or recovery

**Random Events (10% chance per update):**
- **Health Spike**: +10-20% (maintenance/repair)
- **Health Drop**: -15-30% (component failure)
- **System Restoration**: Reset to 85-100% (full service)

## Scripts

### external_system.py

Single subsystem health monitor with full configuration options.

**Basic Usage:**
```bash
python3 external_system.py antenna_1
```

**With Options:**
```bash
python3 external_system.py antenna_1 --interval 3.0 --host localhost --port 12345
```

**Arguments:**
- `component_id` (required): Subsystem ID to monitor (e.g., antenna_1, power_1)
- `--host`: Server hostname (default: localhost)
- `--port`: Server port (default: 12345)
- `--interval`: Health update interval in seconds (default: 2.0)

**Example:**
```bash
# Monitor antenna subsystem with 1.5 second updates
python3 external_system.py antenna_1 --interval 1.5
```

### run_multiple_systems.py

Launches and manages multiple health monitors simultaneously.

**Basic Usage:**
```bash
python3 run_multiple_systems.py
```

**Custom Subsystems:**
```bash
python3 run_multiple_systems.py --components antenna_1 power_1 cooling_1 comm_1 computer_1
```

**With Options:**
```bash
python3 run_multiple_systems.py --components antenna_1 power_1 --interval 3.0 --host localhost --port 12345
```

**Arguments:**
- `--components`: List of subsystem IDs to monitor (default: antenna_1 power_1 cooling_1 comm_1 computer_1)
- `--host`: Server hostname (default: localhost)
- `--port`: Server port (default: 12345)
- `--interval`: Base update interval (default: 2.0)

**Features:**
- Each monitor gets a slightly different interval to avoid synchronization
- Starts all monitors automatically
- Press Ctrl+C to stop all monitors

**Example:**
```bash
# Monitor 3 specific subsystems
python3 run_multiple_systems.py --components antenna_1 power_1 cooling_1 --interval 2.5
```

## Message Format

Health updates are sent as JSON messages:

```json
{
  "component_id": "antenna_1",
  "color": "#00FF00",
  "size": 95.5
}
```

**Fields:**
- `component_id`: Unique identifier for the subsystem
- `color`: Hex color code representing health status
- `size`: Health percentage (0-100)

**Color Mapping:**
- `#00FF00` - Green (Operational)
- `#FFFF00` - Yellow (Warning)
- `#FFA500` - Orange (Degraded)
- `#FF0000` - Red (Critical)
- `#808080` - Gray (Offline)

## Common Subsystem IDs

When creating layouts in the Designer App, subsystems are assigned IDs like:
- `component_1` - First subsystem placed
- `component_2` - Second subsystem placed
- `component_3` - Third subsystem placed
- etc.

For clarity, you can use descriptive names:
- `antenna_1`, `antenna_2` - For antenna subsystems
- `power_1`, `power_2` - For power systems
- `cooling_1` - For cooling units
- `comm_1` - For communication systems
- `computer_1` - For radar computers

## Workflow

### Step 1: Start Runtime Application
```bash
./RuntimeApp/RuntimeApp
# Load a design file
```

### Step 2: Match Subsystem IDs
Note the subsystem IDs shown in the Runtime Application (e.g., component_1, component_2)

### Step 3: Start Health Monitors
```bash
cd ExternalSystems

# Option 1: Use the multi-monitor script (recommended)
python3 run_multiple_systems.py --components component_1 component_2 component_3

# Option 2: Start individual monitors
python3 external_system.py component_1 &
python3 external_system.py component_2 &
python3 external_system.py component_3 &
```

### Step 4: Observe Updates
Watch the Runtime Application as subsystems change color and size based on health status.

## Examples

### Monitor All Five Subsystem Types

```bash
# Using descriptive IDs (if used in design)
python3 run_multiple_systems.py --components antenna_1 power_1 cooling_1 comm_1 computer_1
```

### Fast Updates (Testing)

```bash
# Update every 0.5 seconds
python3 run_multiple_systems.py --interval 0.5
```

### Slow Updates (Demo)

```bash
# Update every 5 seconds
python3 run_multiple_systems.py --interval 5.0
```

### Monitor Specific Subsystems

```bash
# Only monitor critical subsystems
python3 run_multiple_systems.py --components power_1 cooling_1
```

### Single Subsystem with Custom Interval

```bash
# Monitor antenna with 1 second updates
python3 external_system.py antenna_1 --interval 1.0
```

## Console Output

Health monitors provide detailed console output:

```
[antenna_1] Starting Radar Subsystem Health Monitor
[antenna_1] Update interval: 2.0 seconds
[antenna_1] Initial health: 95.0%

[antenna_1] Connected to localhost:12345
[antenna_1] Health Update: status=operational, health=93.2%, color=#00FF00
[antenna_1] Health Update: status=operational, health=91.8%, color=#00FF00
[antenna_1] EVENT: Health drop!
[antenna_1] Health Update: status=warning, health=68.4%, color=#FFFF00
[antenna_1] Health Update: status=warning, health=71.2%, color=#FFFF00
[antenna_1] EVENT: System restored!
[antenna_1] Health Update: status=operational, health=96.5%, color=#00FF00
```

## Creating Custom Health Monitors

You can create your own health monitoring scripts:

```python
import socket
import json
import time

# Connect to Runtime Application
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 12345))

while True:
    # Create health update message
    message = {
        "component_id": "antenna_1",
        "color": "#00FF00",    # Green for operational
        "size": 95.0           # 95% health
    }
    
    # Send message
    sock.sendall((json.dumps(message) + '\n').encode('utf-8'))
    
    # Wait before next update
    time.sleep(2.0)
```

### Custom Health Logic Example

```python
import socket
import json
import time
import random

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 12345))

health = 100.0

while True:
    # Your custom health calculation
    if time.localtime().tm_hour >= 9 and time.localtime().tm_hour < 17:
        # Business hours - higher load, gradual degradation
        health -= random.uniform(0.5, 2.0)
    else:
        # After hours - recovery
        health += random.uniform(0.5, 1.5)
    
    health = max(0.0, min(100.0, health))
    
    # Determine color based on health
    if health >= 90:
        color = "#00FF00"  # Green
    elif health >= 70:
        color = "#FFFF00"  # Yellow
    elif health >= 40:
        color = "#FFA500"  # Orange
    else:
        color = "#FF0000"  # Red
    
    message = {
        "component_id": "antenna_1",
        "color": color,
        "size": health
    }
    
    sock.sendall((json.dumps(message) + '\n').encode('utf-8'))
    time.sleep(2.0)
```

## Troubleshooting

### Connection Refused
**Problem**: `[antenna_1] Connection failed: [Errno 111] Connection refused`

**Solutions:**
- Ensure Runtime Application is running
- Verify a design has been loaded
- Check firewall settings
- Confirm port 12345 is not blocked

### Subsystem Not Updating
**Problem**: Health monitor connects but subsystem doesn't update

**Solutions:**
- Verify subsystem ID matches the one in the loaded design
- Check Runtime Application console for received messages
- Ensure JSON format is correct

### Multiple Monitors Conflict
**Problem**: Multiple monitors for same subsystem

**Solution:**
- Each subsystem should have only one health monitor
- Use unique subsystem IDs in your design

## Advanced Usage

### Remote Monitoring

```bash
# Monitor subsystem on remote server
python3 external_system.py antenna_1 --host 192.168.1.100 --port 12345
```

### Background Execution

```bash
# Run in background (Linux/macOS)
python3 external_system.py antenna_1 > antenna_1.log 2>&1 &

# Multiple monitors in background
for id in antenna_1 power_1 cooling_1; do
    python3 external_system.py $id > ${id}.log 2>&1 &
done
```

### Automated Testing

```bash
#!/bin/bash
# test_radar_health.sh

# Start Runtime App (assuming it's already running)

# Monitor for 60 seconds
timeout 60 python3 run_multiple_systems.py --components antenna_1 power_1

echo "Test completed"
```

## Requirements

- Python 3.x (no external packages required)
- Network access to Runtime Application host
- TCP port 12345 accessible (or custom port)

## Tips

1. **Match IDs carefully**: Subsystem IDs are case-sensitive
2. **Use descriptive IDs**: Name subsystems clearly when designing
3. **Stagger intervals**: Use `run_multiple_systems.py` to avoid simultaneous updates
4. **Monitor logs**: Check console output for connection issues
5. **Test connectivity**: Start with one monitor before running multiple

## See Also

- [Main README](../README.md) - Full system documentation
- [Quick Start Guide](../QUICKSTART.md) - Get started in 5 minutes
- Runtime Application documentation for server setup
- Designer Application documentation for creating layouts
