# External Radar Subsystem Health Monitors

Python scripts that simulate external radar subsystems sending periodic health status updates to the UnifiedApp.

## Overview

These health monitors simulate real-world radar subsystems that report their operational status periodically. Each monitor:
- Connects to the UnifiedApp via TCP (port 12345)
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

### apcu_simulator.py (APCU Antenna System Simulator)

Full-fidelity simulator for the APCU 4 Left antenna system. Replicates all
subsystems visible in the APCU control interface:

- **APCU Controller** - Link status (Q0-Q3), Sequence On/Off, Array Voltage/Current
- **Temperature Monitoring** - 3 Sensors (A, B, C) in Deg C
- **Board Data** - Input Voltage (V), Current (A)
- **Board Status** - CRC checks (User/Factory/Program Flash), CBIT, PBIT, Board Setting Check
- **Quadrant 0 (CB)** - 16 channels with On/Off, Trip, Bit Status, Voltage, Current
- **Quadrant 1 (CF)** - 16 channels
- **Quadrant 2 (CD)** - 16 channels
- **Quadrant 3 (CE)** - 16 channels
- **QTRMs** - Quad Transmit/Receive Module groupings
- **AQC** - Antenna Quadrant Controllers (0,1,2,3)

**Basic Usage:**
```bash
python3 apcu_simulator.py component_1
```

**With Options:**
```bash
python3 apcu_simulator.py antenna_1 --mode degraded --interval 1.5
```

**Arguments:**
- `component_id` (required): Component ID on canvas (e.g., component_1)
- `--host`: Server hostname (default: localhost)
- `--port`: Server port (default: 12345)
- `--interval`: Update interval in seconds (default: 2.0)
- `--mode`: Simulation mode: `nominal`, `degraded`, or `critical` (default: nominal)
- `--quiet`: Suppress verbose console output
- `--no-telemetry`: Send basic health only (no full APCU telemetry)

**Console Output:**
```
══════════════════════════════════════════════════════════════════════
  APCU 4 Left [V1.0.0]   Component: antenna_1
══════════════════════════════════════════════════════════════════════
  Array Voltage:  27.85 V   Array Current: 22.35 A   Overall Health:  94.2% [OPERATIONAL]
  Links: Q0=UP  Q1=UP  Q2=UP  Q3=UP  Seq: ON

  Temperature:  A=41.8°C  B=49.1°C  C=42.8°C  [Normal]
  Board Data:   Vin=27.85V  Iin=5.33A
  Board Status: CRC_UF=Pass  CRC_FF=Pass  CRC_PF=Pass  CBIT=Pass  PBIT=Pass  BSC=Pass

  Quadrant 0 (CB)  Active: 16/16  Tripped: 0  Faulted: 0  Health: 100.0%
  Quadrant 1 (CF)  Active: 16/16  Tripped: 0  Faulted: 0  Health: 100.0%
  Quadrant 2 (CD)  Active: 16/16  Tripped: 0  Faulted: 0  Health: 100.0%
  Quadrant 3 (CE)  Active: 16/16  Tripped: 0  Faulted: 0  Health: 100.0%

  QTRMs Health: 100.0%  AQC Health: 100.0%
```

### external_system.py

Single subsystem health monitor with full configuration options. Supports
optional per-subsystem health reporting for known component types.

**Basic Usage:**
```bash
python3 external_system.py antenna_1
```

**With Subsystem-Level Health (Antenna type):**
```bash
python3 external_system.py antenna_1 --type Antenna
```

**With Options:**
```bash
python3 external_system.py antenna_1 --interval 3.0 --host localhost --port 12345
```

**Arguments:**
- `component_id` (required): Subsystem ID to monitor (e.g., antenna_1, power_1)
- `--host`: Server hostname (default: localhost)
- `--port`: Server port (default: 12345)
- `--protocol`: tcp or udp (default: tcp)
- `--interval`: Health update interval in seconds (default: 2.0)
- `--type`: Component type for subsystem health (Antenna, PowerSystem, LiquidCoolingUnit, CommunicationSystem, RadarComputer)

**Example:**
```bash
# Monitor antenna subsystem with 1.5 second updates
python3 external_system.py antenna_1 --interval 1.5

# Monitor with subsystem detail
python3 external_system.py power_1 --type PowerSystem
```

### run_multiple_systems.py

Launches and manages multiple health monitors simultaneously. Automatically
selects the appropriate simulator based on component ID prefixes.

**Basic Usage:**
```bash
python3 run_multiple_systems.py
```

**With APCU Antenna Simulator:**
```bash
python3 run_multiple_systems.py --components antenna_1 power_1 cooling_1 --apcu antenna_1
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
- `--components`: List of subsystem IDs to monitor (default: component_1 through component_5)
- `--apcu`: Component IDs that should use the APCU antenna simulator
- `--host`: Server hostname (default: localhost)
- `--port`: Server port (default: 12345)
- `--protocol`: tcp or udp (default: tcp)
- `--interval`: Base update interval (default: 2.0)

**Automatic Simulator Selection:**
- Components in `--apcu` list -> `apcu_simulator.py`
- Component IDs starting with `antenna` -> `apcu_simulator.py`
- Component IDs starting with `power` -> `external_system.py --type PowerSystem`
- Component IDs starting with `cooling` -> `external_system.py --type LiquidCoolingUnit`
- Component IDs starting with `comm` -> `external_system.py --type CommunicationSystem`
- Component IDs starting with `computer` -> `external_system.py --type RadarComputer`
- All others -> `external_system.py` (generic)

**Features:**
- Each monitor gets a slightly different interval to avoid synchronization
- Starts all monitors automatically
- Press Ctrl+C to stop all monitors

**Example:**
```bash
# Monitor 3 specific subsystems with APCU antenna
python3 run_multiple_systems.py --components antenna_1 power_1 cooling_1 --apcu antenna_1 --interval 2.5
```

## Message Protocol

### Basic Health Update

```json
{
  "component_id": "antenna_1",
  "color": "#00FF00",
  "size": 95.5
}
```

### Subsystem-Level Health Update

```json
{
  "component_id": "antenna_1",
  "subsystem": "Quadrant 0 (CB)",
  "color": "#00FF00",
  "size": 98.2
}
```

### Bulk Subsystem Health (in component update)

```json
{
  "component_id": "antenna_1",
  "color": "#00FF00",
  "size": 95.5,
  "subsystem_health": {
    "APCU Controller": 100.0,
    "Temperature Monitoring": 95.0,
    "Board Data": 100.0,
    "Board Status": 100.0,
    "Quadrant 0 (CB)": 98.2,
    "Quadrant 1 (CF)": 97.5,
    "Quadrant 2 (CD)": 99.1,
    "Quadrant 3 (CE)": 96.8,
    "QTRMs": 97.9,
    "AQC": 98.5
  }
}
```

### Full APCU Telemetry

```json
{
  "component_id": "antenna_1",
  "color": "#00FF00",
  "size": 95.5,
  "subsystem_health": { ... },
  "apcu_telemetry": {
    "version": "1.0.0",
    "unit": "APCU 4 Left",
    "link_status": { "Q0": true, "Q1": true, "Q2": true, "Q3": true },
    "sequence_on": true,
    "array_voltage": 27.87,
    "array_current": 22.35,
    "temperature": {
      "sensor_a": 41.79,
      "sensor_b": 49.10,
      "sensor_c": 42.78,
      "status": "Normal"
    },
    "board_data": { "input_voltage": 27.85, "current": 5.33 },
    "mode_status": {
      "operating": "Operating",
      "loss_of_input_voltage": "No Loss",
      "reset_source": "Software Reset"
    },
    "board_status": {
      "crc_user_flash": "Pass",
      "crc_factory_flash": "Pass",
      "crc_program_flash": "Pass",
      "cbit": "Pass",
      "pbit": "Pass",
      "board_setting_check": "Pass"
    },
    "quadrants": [
      {
        "name": "Quadrant 0 (CB)",
        "code": "CB",
        "health": 98.2,
        "active": 16,
        "tripped": 0,
        "faulted": 0,
        "channels": [
          { "ch": 0, "on": true, "trip": "Normal", "bit": "Normal", "voltage": 27.95, "current": 0.16 },
          ...
        ]
      },
      ...
    ],
    "qtrm_groups": [ [5,8,15,18,25,28], ... ],
    "aqc_controllers": [0, 1, 2, 3]
  }
}
```

**Fields:**
- `component_id`: Unique identifier for the subsystem
- `color`: Hex color code representing health status
- `size`: Health percentage (0-100)
- `subsystem`: (optional) Name of specific subsystem being reported
- `subsystem_health`: (optional) Map of subsystem names to health percentages
- `apcu_telemetry`: (optional) Full APCU telemetry payload

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

### Step 1: Start the UnifiedApp
```bash
./UnifiedApp/UnifiedApp
# Login with User/user for runtime mode, then load a design file
```

### Step 2: Match Subsystem IDs
Note the subsystem IDs shown in the application (e.g., component_1, component_2)

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
Watch the UnifiedApp as subsystems change color and size based on health status.

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
- Ensure UnifiedApp is running
- Verify a design has been loaded
- Check firewall settings
- Confirm port 12345 is not blocked

### Subsystem Not Updating
**Problem**: Health monitor connects but subsystem doesn't update

**Solutions:**
- Verify subsystem ID matches the one in the loaded design
- Check UnifiedApp console for received messages
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
- Network access to UnifiedApp host
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
- [UnifiedApp README](../UnifiedApp/README.md) - Application documentation
