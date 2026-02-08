# Quick Start Guide - Radar System Monitoring

Get up and running with the Radar System Monitoring application in 5 minutes!

## Prerequisites

- Qt 5.x or Qt 6.x installed
- Python 3.x installed
- Linux/macOS (Windows also supported)

## Step 1: Build the Applications (1 minute)

```bash
cd /workspace
./build_all.sh
```

This builds:
- Designer Application for creating radar system layouts
- Runtime Application for real-time health monitoring

## Step 2: Design Your Radar System (1 minute)

```bash
./DesignerApp/DesignerApp
```

1. **Drag subsystems** from the left panel to the canvas:
   - Antenna
   - Power System
   - Liquid Cooling Unit
   - Communication System
   - Radar Computer

2. **Position them** to represent your radar system layout

3. **Save the design**:
   - Click "Save Design"
   - Save as `radar_system.design`
   - Note the subsystem IDs shown (e.g., antenna_1, power_1)

## Step 3: Start the Monitoring Application (30 seconds)

```bash
./RuntimeApp/RuntimeApp
```

1. Click "Load Design"
2. Select `radar_system.design`
3. The server starts automatically on port 12345

## Step 4: Start Health Monitors (30 seconds)

```bash
cd ExternalSystems
python3 run_multiple_systems.py --components antenna_1 power_1 cooling_1 comm_1 computer_1
```

Or start individual monitors:
```bash
python3 external_system.py antenna_1 &
python3 external_system.py power_1 &
python3 external_system.py cooling_1 &
```

## Step 5: Watch the Magic! (ongoing)

You should now see:
- **Subsystems changing colors** based on health status:
  - Green = Operational (90-100%)
  - Yellow = Warning (70-89%)
  - Orange = Degraded (40-69%)
  - Red = Critical (10-39%)
  - Gray = Offline (0-9%)

- **Analytics updating** in real-time:
  - Health update counts
  - Current status and health levels
  - Change statistics

## Understanding the Health Simulation

The external health monitors simulate realistic radar subsystem behavior:

- **Gradual health changes**: Health slowly increases or decreases
- **Random events** (10% chance per update):
  - Health spike (recovery)
  - Health drop (failure)
  - System restoration (full recovery)

## Customization

### Change Update Interval

```bash
# Update every 5 seconds instead of 2
python3 run_multiple_systems.py --interval 5.0
```

### Monitor Specific Subsystems

```bash
# Only monitor antenna and power system
python3 run_multiple_systems.py --components antenna_1 power_1
```

### Use Different Subsystem IDs

In Designer App, each subsystem gets an ID like `component_1`, `component_2`, etc.
Match these IDs when starting health monitors:

```bash
python3 external_system.py component_1  # For first subsystem
python3 external_system.py component_2  # For second subsystem
```

## System Architecture at a Glance

```
Designer App                Runtime Monitor            External Monitors
    │                            │                           │
    │ 1. Create layout           │                           │
    │ ──────────────────>        │                           │
    │                            │                           │
    │ 2. Save design             │                           │
    │ ──────────>                │                           │
    │           design.file      │                           │
    │                            │ 3. Load design            │
    │                            │ <───────────              │
    │                            │                           │
    │                            │ 4. Start TCP:12345        │
    │                            │ <═══════════════════════> │
    │                            │                           │
    │                            │ 5. Health updates         │
    │                            │ <─────────────────────── │
    │                            │                           │
    │                            │ 6. Update display         │
    │                            │ ─────────                │
```

## Message Protocol

Health updates are sent as JSON via TCP:

```json
{
  "component_id": "antenna_1",
  "color": "#00FF00",
  "size": 95.5
}
```

- **component_id**: Subsystem identifier
- **color**: Health status color (green/yellow/orange/red/gray)
- **size**: Health percentage (0-100)

## Troubleshooting

### Applications won't build
```bash
# Install Qt
sudo apt-get install qt5-qmake qtbase5-dev qtbase5-dev-tools libqt5network5
```

### Health monitors can't connect
- Make sure Runtime App is running
- Make sure you've loaded a design
- Check that subsystem IDs match

### No updates showing
- Verify subsystem IDs match between design and monitors
- Check External Systems console for error messages
- Ensure Runtime App shows "Clients: N" where N > 0

## Next Steps

- **Custom layouts**: Design complex radar system architectures
- **Custom health data**: Write your own health monitoring scripts
- **Real integration**: Connect to actual radar subsystems
- **Extended analytics**: Track historical health trends

## Quick Commands Reference

```bash
# Build everything
./build_all.sh

# Clean build artifacts
./clean_all.sh

# Run Designer
./DesignerApp/DesignerApp

# Run Runtime Monitor
./RuntimeApp/RuntimeApp

# Run all health monitors
cd ExternalSystems && python3 run_multiple_systems.py

# Run single health monitor
python3 external_system.py antenna_1

# Custom interval
python3 external_system.py antenna_1 --interval 5.0
```

## Have Fun Monitoring!

You now have a fully functional Radar System Monitoring application. Experiment with:
- Different subsystem layouts
- Multiple subsystems
- Custom health patterns
- Integration with real data sources

For more details, see the full [README.md](README.md).
