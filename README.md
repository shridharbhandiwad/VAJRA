# Radar System Monitoring Application

A comprehensive Qt-based application system for designing, deploying, and monitoring radar subsystems in real-time. This system provides both separate and unified application options:

## Available Applications

### **UnifiedApp** (Recommended)
A single, unified application combining both Designer and Runtime capabilities with role-based access control:
- Login as **Designer** (`Designer`/`designer`) for full design capabilities
- Login as **User** (`User`/`user`) for runtime monitoring
- One application to install and maintain
- Consistent user experience across modes
- See `UnifiedApp/README.md` for details

### **Separate Applications** (Legacy)
Original standalone applications:
1. **DesignerApp** - Visual editor for creating radar system layouts
2. **RuntimeApp** - Real-time health monitoring display for radar subsystems

### **External Subsystem Simulators**
Python scripts that simulate radar subsystems sending health status updates (works with both UnifiedApp and separate apps)

## System Overview

### Designer Application

The Designer Application provides a visual interface for creating radar system layouts with:
- **Radar Subsystems Panel** - Draggable subsystem list (Antenna, Power System, Liquid Cooling Unit, Communication System, Radar Computer)
- **Designer View** - Canvas for placing and arranging subsystems
- **Analytics Panel** - Statistics about subsystems on the canvas
- **Save/Load** - Export designs to `.design` files

### Runtime Application

The Runtime Application is the monitoring application that:
- Loads layouts created by the Designer Application
- Receives health status messages from external subsystems via TCP socket (port 12345)
- Updates subsystem visual indicators based on health status
- Tracks analytics: health updates, status changes, health level changes
- Displays real-time health status with color coding:
  - **Green** - Operational (90-100% health)
  - **Yellow** - Warning (70-89% health)
  - **Orange** - Degraded (40-69% health)
  - **Red** - Critical (10-39% health)
  - **Gray** - Offline (0-9% health)

### External Subsystem Simulators

Python simulators that:
- Connect to the Runtime Application via TCP
- Send periodic health status messages
- Simulate realistic health degradation and recovery
- Generate random events (health spikes, drops, system restoration)
- Each external system monitors one subsystem (by ID)

## Architecture

```
┌─────────────────────┐
│ Designer App        │
│                     │
│  ┌──────────────┐   │
│  │ Subsystems   │   │
│  │ - Antenna    │   │
│  │ - Power      │   │
│  │ - Cooling    │   │
│  │ - Comm       │   │
│  │ - Computer   │   │
│  └──────────────┘   │
│                     │
│  ┌──────────────┐   │
│  │ Canvas       │   │
│  │ (drag/drop)  │   │
│  └──────────────┘   │
│                     │
│  ┌──────────────┐   │
│  │ Analytics    │   │
│  └──────────────┘   │
│                     │
│  [Save Design]      │
└─────────────────────┘
         │
         ▼
    design.file
         │
         ▼
┌─────────────────────┐       ┌─────────────────────┐
│ Runtime Monitor     │◄──────│ External Subsystems │
│                     │ TCP   │                     │
│  ┌──────────────┐   │:12345 │  ┌──────────────┐   │
│  │ Radar View   │◄──┼───────┤  │ Antenna      │   │
│  │ (real-time)  │   │       │  │ Health Mon.  │   │
│  └──────────────┘   │       │  └──────────────┘   │
│                     │       │                     │
│  ┌──────────────┐   │       │  ┌──────────────┐   │
│  │ Health       │   │       │  │ Power System │   │
│  │ Analytics    │   │       │  │ Health Mon.  │   │
│  └──────────────┘   │       │  └──────────────┘   │
│                     │       │                     │
│  [Load Design]      │       │  ┌──────────────┐   │
└─────────────────────┘       │  │ Cooling Unit │   │
                              │  │ Health Mon.  │   │
                              │  └──────────────┘   │
                              │                     │
                              │  ┌──────────────┐   │
                              │  │ Comm System  │   │
                              │  │ Health Mon.  │   │
                              │  └──────────────┘   │
                              │                     │
                              │  ┌──────────────┐   │
                              │  │ Radar Comp.  │   │
                              │  │ Health Mon.  │   │
                              │  └──────────────┘   │
                              └─────────────────────┘
```

## Requirements

### For Qt Applications
- Qt 5.x or Qt 6.x
- C++ compiler (g++, clang, or MSVC)
- Qt development tools (qmake, Qt Creator optional)

### For External Subsystem Simulators
- Python 3.x (standard library only, no external packages needed)

## Building

### Quick Start (Unified Application)

```bash
cd UnifiedApp
qmake UnifiedApp.pro
make
./UnifiedApp
```

Login with `Designer`/`designer` for design mode or `User`/`user` for runtime mode.

### Build All Applications (Including Legacy Apps)

```bash
# Make scripts executable
chmod +x build_all.sh clean_all.sh

# Build all applications (DesignerApp, RuntimeApp, and UnifiedApp)
./build_all.sh
```

### Windows

```cmd
# Build Designer Application
cd DesignerApp
qmake DesignerApp.pro
nmake  # or mingw32-make
cd ..

# Build Runtime Application
cd RuntimeApp
qmake RuntimeApp.pro
nmake  # or mingw32-make
cd ..
```

### Using Qt Creator

1. Open `DesignerApp/DesignerApp.pro` in Qt Creator
2. Build and run
3. Open `RuntimeApp/RuntimeApp.pro` in Qt Creator
4. Build and run

## Usage

### Complete Workflow

#### Using UnifiedApp (Recommended)

**Step 1: Design Your Radar System Layout**

```bash
# Run Unified Application
./UnifiedApp/UnifiedApp
```

1. Login with `Designer`/`designer`
2. Drag subsystems (Antenna, Power System, etc.) from the left panel to the canvas
3. Position subsystems as desired to represent your radar system layout
4. Note the subsystem IDs (e.g., component_1, component_2, etc.)
5. Click "Save Design" and save as `radar_system.design`

**Step 2: Run Runtime Monitoring**

```bash
# Run Unified Application (or keep it open from Step 1)
./UnifiedApp/UnifiedApp
```

1. Login with `User`/`user`
2. Click "Load Design"
3. Select your saved `radar_system.design` file
4. The canvas will display your radar subsystems
5. The server automatically starts listening on port 12345 for health updates

#### Using Separate Applications (Legacy)

**Step 1: Design Your Radar System Layout**

```bash
# Run Designer Application
./DesignerApp/DesignerApp
```

1. Drag subsystems (Antenna, Power System, etc.) from the left panel to the canvas
2. Position subsystems as desired to represent your radar system layout
3. Note the subsystem IDs (e.g., antenna_1, power_1, cooling_1, comm_1, computer_1)
4. Click "Save Design" and save as `radar_system.design`

**Step 2: Run Runtime Monitoring Application**

```bash
# Run Runtime Application
./RuntimeApp/RuntimeApp
```

1. Click "Load Design"
2. Select your saved `radar_system.design` file
3. The canvas will display your radar subsystems
4. The server will start listening on port 12345 for health updates

#### Step 3: Start Subsystem Health Monitors

```bash
cd ExternalSystems

# Option 1: Run individual health monitors
python3 external_system.py antenna_1 &
python3 external_system.py power_1 &
python3 external_system.py cooling_1 &
python3 external_system.py comm_1 &
python3 external_system.py computer_1 &

# Option 2: Run all monitors at once (recommended)
python3 run_multiple_systems.py --components antenna_1 power_1 cooling_1 comm_1 computer_1
```

#### Step 4: Watch Real-Time Health Monitoring

- Subsystems will change color based on health status
- The Analytics panel shows:
  - Health update counts per subsystem
  - Current status color and health level
  - Number of status and level changes
  - Total health updates across all subsystems

## Health Status Protocol

External subsystems communicate with the Runtime Application via TCP socket using JSON messages:

### Message Format

```json
{
  "component_id": "antenna_1",
  "color": "#00FF00",
  "size": 95.5
}
```

### Health Status Mapping

| Health Level | Status | Color | Description |
|-------------|--------|-------|-------------|
| 90-100% | Operational | Green (#00FF00) | Normal operation |
| 70-89% | Warning | Yellow (#FFFF00) | Minor issues detected |
| 40-69% | Degraded | Orange (#FFA500) | Performance degraded |
| 10-39% | Critical | Red (#FF0000) | Critical issues |
| 0-9% | Offline | Gray (#808080) | System offline |

### Fields
- `component_id` (string): ID of the subsystem to update
- `color` (string): Hex color code representing health status
- `size` (number): Health percentage (0-100)

### Connection
- **Protocol**: TCP
- **Port**: 12345
- **Format**: JSON, one message per line (newline-delimited)

## Radar Subsystems

### Antenna
- Primary radar signal transmission and reception
- Health indicators: signal strength, alignment, mechanical status
- Visual: Dish antenna with support structure

### Power System
- Provides power to all radar components
- Health indicators: voltage levels, current draw, battery status
- Visual: Battery/power unit with lightning bolt

### Liquid Cooling Unit
- Maintains optimal temperature for radar components
- Health indicators: coolant temperature, flow rate, pump status
- Visual: Cooling system with pipes and radiators

### Communication System
- Handles data transmission and network connectivity
- Health indicators: signal quality, bandwidth, link status
- Visual: Radio tower with signal waves

### Radar Computer
- Processes radar data and controls system operations
- Health indicators: CPU usage, memory, processing capacity
- Visual: Computer unit with circuit patterns

## Examples

### Create a Radar System Layout

1. Start Designer Application
2. Drag one of each subsystem type to canvas
3. Arrange them to represent your radar system architecture
4. Save as `my_radar.design`

### Monitor with Simulated Health Data

```bash
# Terminal 1: Start Runtime Monitor
./RuntimeApp/RuntimeApp
# Load my_radar.design

# Terminal 2: Start health simulators
cd ExternalSystems
python3 run_multiple_systems.py --components antenna_1 power_1 cooling_1 comm_1 computer_1
```

Watch the subsystems change colors and sizes based on simulated health data!

### Custom Health Monitor

Create your own health monitor by connecting to port 12345 and sending JSON messages:

```python
import socket
import json
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 12345))

while True:
    # Simulate operational status
    message = {
        "component_id": "antenna_1",
        "color": "#00FF00",  # Green - operational
        "size": 95.0         # 95% health
    }
    sock.sendall((json.dumps(message) + '\n').encode('utf-8'))
    time.sleep(2)
```

## Project Structure

```
.
├── UnifiedApp/               # Unified Application (RECOMMENDED)
│   ├── UnifiedApp.pro        # Qt project file
│   ├── main.cpp
│   ├── logindialog.cpp/h     # Login dialog with role-based auth
│   ├── mainwindow.cpp/h      # Main window (Designer + Runtime modes)
│   ├── component.cpp/h       # Subsystem graphics item
│   ├── canvas.cpp/h          # Unified canvas widget
│   ├── componentlist.cpp/h   # Subsystem list with drag/drop
│   ├── analytics.cpp/h       # Analytics widget
│   ├── messageserver.cpp/h   # TCP server for health updates
│   ├── README.md             # Detailed documentation
│   └── QUICKSTART.md         # Quick start guide
│
├── DesignerApp/              # Designer Application (Legacy)
│   ├── DesignerApp.pro       # Qt project file
│   ├── main.cpp
│   ├── mainwindow.cpp/h      # Main window with UI
│   ├── component.cpp/h       # Subsystem graphics item
│   ├── canvas.cpp/h          # Canvas widget
│   ├── componentlist.cpp/h   # Subsystem list with drag/drop
│   └── analytics.cpp/h       # Analytics widget
│
├── RuntimeApp/               # Runtime Monitoring Application (Legacy)
│   ├── RuntimeApp.pro        # Qt project file
│   ├── main.cpp
│   ├── mainwindow.cpp/h      # Main window with UI
│   ├── component.cpp/h       # Subsystem graphics item
│   ├── canvas.cpp/h          # Canvas widget
│   ├── analytics.cpp/h       # Health analytics widget
│   └── messageserver.cpp/h   # TCP server for health updates
│
├── ExternalSystems/          # Subsystem health simulators
│   ├── external_system.py    # Single subsystem health monitor
│   ├── run_multiple_systems.py  # Multi-monitor launcher
│   └── README.md
│
├── build_all.sh              # Build script (Linux/macOS)
├── clean_all.sh              # Clean script (Linux/macOS)
└── README.md                 # This file
```

## Features

### Designer Application Features
- ✓ Drag-and-drop subsystem placement
- ✓ Five subsystem types: Antenna, Power System, Liquid Cooling Unit, Communication System, Radar Computer
- ✓ Save/Load system layouts
- ✓ Subsystem analytics
- ✓ Visual subsystem icons

### Runtime Application Features
- ✓ Load layouts from Designer App
- ✓ TCP server for external health monitoring
- ✓ Real-time subsystem health updates
- ✓ Multi-client support (multiple subsystems simultaneously)
- ✓ Color-coded health status
- ✓ Comprehensive health analytics
- ✓ Connection status monitoring

### External Subsystem Features
- ✓ Realistic health simulation with degradation
- ✓ Random event generation (spikes, drops, restoration)
- ✓ Configurable update intervals
- ✓ Command-line configuration
- ✓ Multi-subsystem manager
- ✓ Automatic reconnection

## Troubleshooting

### Build Issues

**Problem**: `qmake: command not found`
```bash
# Ubuntu/Debian
sudo apt-get install qt5-qmake qtbase5-dev qtbase5-dev-tools libqt5network5

# Fedora
sudo dnf install qt5-qtbase-devel

# macOS
brew install qt@5
```

**Problem**: Network headers not found
```bash
# Ubuntu/Debian
sudo apt-get install qtbase5-dev libqt5network5
```

### Runtime Issues

**Problem**: Server fails to start
- Check if port 12345 is already in use: `lsof -i :12345`
- Try a different port (requires code modification)

**Problem**: Health monitors can't connect
- Ensure Runtime Application is running and loaded a design
- Check firewall settings
- Verify correct host and port

**Problem**: Subsystems not updating
- Verify subsystem IDs match between design and health monitors
- Check external system console for connection status
- Ensure messages are properly formatted JSON

## Extending the System

### Add New Subsystem Types

1. Update `ComponentType` enum in `component.h`
2. Add rendering code in `Component::paint()`
3. Add to subsystem list in `componentlist.cpp`
4. Update type name conversions

### Change Communication Protocol

Modify `messageserver.cpp` to support different protocols:
- UDP instead of TCP
- WebSocket for web integration
- MQTT for IoT scenarios
- gRPC for microservices

### Add More Health Metrics

Enhance the health monitoring system:
- Temperature sensors
- Vibration analysis
- Power consumption tracking
- Network latency
- Error rates

### Enhanced Analytics

Upgrade `analytics.cpp` to include:
- Time-series graphs
- Health history tracking
- Predictive maintenance alerts
- Export to CSV/JSON
- Historical trends

## License

This project is provided as-is for educational and demonstration purposes.

## Author

Created as a Qt-based Radar System Monitoring demonstration.
