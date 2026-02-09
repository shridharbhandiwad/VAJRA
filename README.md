# Radar System Monitoring Application

A comprehensive Qt-based application system for designing, deploying, and monitoring radar subsystems in real-time. Features a **fully modular architecture** where new component types can be added without changing any code.

## Key Feature: Modular Component Architecture

**Add new component types without touching a single line of C++ code!**

Components are defined in `components.json` and can be added at runtime through the UI:

```json
{
  "type_id": "GpsReceiver",
  "display_name": "GPS Receiver",
  "label": "GPS",
  "description": "Satellite navigation and positioning",
  "image_dir": "gps_receiver",
  "subsystems": ["Signal Strength", "Satellite Lock", "Position Accuracy"],
  "protocol": "TCP",
  "port": 12345,
  "category": "Navigation",
  "shape": "hexagon"
}
```

Or use the **"+ Add Component Type"** button in the Designer to add new types via the UI.

## Available Applications

### **UnifiedApp** (Recommended - v3.0)
A single, unified application combining both Designer and Runtime capabilities with role-based access control:
- Login as **Designer** (`Designer`/`designer`) for full design capabilities
- Login as **User** (`User`/`user`) for runtime monitoring
- **Modular component registry** - add new component types via JSON or UI
- **Multi-protocol support** - TCP and UDP for health data
- **Modern glass-morphism UI** with teal accent theme
- See `UnifiedApp/README.md` for details

### **Separate Applications** (Legacy)
Original standalone applications:
1. **DesignerApp** - Visual editor for creating radar system layouts
2. **RuntimeApp** - Real-time health monitoring display for radar subsystems

### **External Subsystem Simulators**
Python scripts that simulate subsystems sending health status updates (works with both UnifiedApp and separate apps)

## System Overview

### Designer Mode

The Designer provides a visual interface for creating system layouts with:
- **Component Panel** - Draggable list of all registered component types
- **"+ Add Component Type"** - Button to define new component types at runtime
- **Designer View** - Canvas for placing and arranging subsystems
- **Analytics Panel** - Statistics about subsystems on the canvas
- **Save/Load** - Export designs to `.design` files

### Runtime Mode

The Runtime Monitor:
- Loads layouts created by the Designer
- Receives health status messages via TCP (port 12345) and UDP (port 12346)
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
- Connect via TCP or UDP
- Send periodic health status messages in JSON format
- Simulate realistic health degradation and recovery
- Work with any component type (just pass the canvas component ID)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    components.json                           │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │ Antenna  │ │ Power    │ │ Cooling  │ │ Custom!  │ ...    │
│  │ ANT/TCP  │ │ PWR/TCP  │ │ COOL/TCP │ │ GPS/UDP  │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
└──────────────────────┬──────────────────────────────────────┘
                       │ loaded at startup
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  UnifiedApp (Qt C++)                                        │
│                                                              │
│  ComponentRegistry ──→ ComponentList ──→ Canvas              │
│        │                                   │                 │
│        │              ┌───────────────┐    │                 │
│        └─────────────→│ AddComponent  │    │                 │
│                       │ Dialog (UI)   │    │                 │
│                       └───────────────┘    │                 │
│                                            ▼                 │
│  MessageServer (TCP:12345, UDP:12346) ──→ Analytics          │
└──────────────────────┬──────────────────────────────────────┘
                       │ TCP / UDP
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  External Subsystem Simulators (Python)                      │
│                                                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                    │
│  │ comp_1   │ │ comp_2   │ │ comp_N   │  (any component)   │
│  │ TCP/UDP  │ │ TCP/UDP  │ │ TCP/UDP  │                    │
│  └──────────┘ └──────────┘ └──────────┘                    │
└─────────────────────────────────────────────────────────────┘
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
chmod +x build_all.sh clean_all.sh
./build_all.sh
```

### Installing Qt (if needed)

```bash
# Ubuntu/Debian
sudo apt-get install qt5-qmake qtbase5-dev qtbase5-dev-tools libqt5network5

# Fedora
sudo dnf install qt5-qtbase-devel

# macOS
brew install qt@5
```

## Usage

### Adding a New Component Type (No Code Changes!)

**Option 1: Via the UI**
1. Run UnifiedApp, login as Designer
2. Click **"+ Add Component Type"** in the toolbar
3. Fill in: name, label, image, subsystems, protocol, port
4. Click "Add Component" - it's immediately available!

**Option 2: Edit components.json**
```json
{
  "type_id": "GpsReceiver",
  "display_name": "GPS Receiver",
  "label": "GPS",
  "description": "Satellite navigation and positioning",
  "image_dir": "gps_receiver",
  "icon_color": "#2196F3",
  "subsystems": ["Signal Strength", "Satellite Lock", "Position Accuracy"],
  "protocol": "UDP",
  "port": 12346,
  "category": "Navigation",
  "shape": "hexagon"
}
```

**Option 3: Add image for the component**
1. Create directory: `assets/subsystems/gps_receiver/`
2. Add image: `assets/subsystems/gps_receiver/gps_receiver_main.jpg`
3. The component will display the image on the canvas automatically

### Complete Workflow

**Step 1: Design Your System Layout**

```bash
./UnifiedApp/UnifiedApp
```

1. Login with `Designer`/`designer`
2. Drag components from the left panel to the canvas
3. Use "+" to add custom component types if needed
4. Save as `radar_system.design`

**Step 2: Run Runtime Monitoring**

1. Login with `User`/`user` (or keep app open)
2. Load your saved design
3. The server starts on port 12345 (TCP) and 12346 (UDP)

**Step 3: Start Health Monitors**

```bash
cd ExternalSystems

# Monitor via TCP (default)
python3 run_multiple_systems.py --components component_1 component_2 component_3

# Monitor via UDP
python3 run_multiple_systems.py --protocol udp --components component_1 component_2

# Monitor individual components
python3 external_system.py component_1
python3 external_system.py component_2 --protocol udp --port 12346
```

## Health Status Protocol

External systems communicate via TCP or UDP using JSON messages:

### Message Format

```json
{
  "component_id": "component_1",
  "color": "#00FF00",
  "size": 95.5
}
```

### Supported Protocols

| Protocol | Port (default) | Format | Notes |
|----------|---------------|--------|-------|
| TCP | 12345 | Line-delimited JSON | Default, reliable delivery |
| UDP | 12346 | JSON datagrams | Lower latency, fire-and-forget |
| WebSocket | - | JSON | Available in registry, extensible |
| MQTT | - | JSON | Available in registry, extensible |

### Health Status Mapping

| Health Level | Status | Color | Description |
|-------------|--------|-------|-------------|
| 90-100% | Operational | Green (#00FF00) | Normal operation |
| 70-89% | Warning | Yellow (#FFFF00) | Minor issues detected |
| 40-69% | Degraded | Orange (#FFA500) | Performance degraded |
| 10-39% | Critical | Red (#FF0000) | Critical issues |
| 0-9% | Offline | Gray (#808080) | System offline |

## Component Registry (components.json)

The component registry is the heart of the modular architecture:

```json
{
  "version": "2.0",
  "components": [
    {
      "type_id": "Antenna",          // Unique identifier
      "display_name": "Antenna",     // Human-readable name
      "label": "ANT",                // Short label on canvas
      "description": "...",          // Component description
      "image_dir": "antenna",        // Image directory name
      "icon_color": "#4CAF50",       // Fallback color
      "subsystems": [...],           // Sub-components for health tracking
      "protocol": "TCP",             // Health data protocol
      "port": 12345,                 // Protocol port
      "category": "Sensor",          // Grouping category
      "shape": "ellipse"             // Fallback geometric shape
    }
  ]
}
```

### Default Component Types

| Type | Label | Category | Shape | Protocol |
|------|-------|----------|-------|----------|
| Antenna | ANT | Sensor | Ellipse | TCP |
| Power System | PWR | Infrastructure | Rect | TCP |
| Liquid Cooling Unit | COOL | Infrastructure | Ellipse | TCP |
| Communication System | COMM | Network | Rect | TCP |
| Radar Computer | CPU | Processing | Rect | TCP |

## Project Structure

```
.
├── UnifiedApp/                  # Unified Application (RECOMMENDED)
│   ├── UnifiedApp.pro           # Qt project file
│   ├── components.json          # Component registry (edit to add types!)
│   ├── main.cpp                 # App entry, initializes registry
│   ├── componentregistry.h/cpp  # Modular component registry
│   ├── addcomponentdialog.h/cpp # UI for adding new types
│   ├── logindialog.h/cpp        # Login dialog with role-based auth
│   ├── mainwindow.h/cpp         # Main window (Designer + Runtime)
│   ├── component.h/cpp          # Data-driven component graphics item
│   ├── canvas.h/cpp             # Canvas with dynamic type resolution
│   ├── componentlist.h/cpp      # Auto-populated component list
│   ├── analytics.h/cpp          # Dynamic analytics widget
│   ├── messageserver.h/cpp      # Multi-protocol health server (TCP+UDP)
│   ├── styles.qss               # Modern glass-morphism theme
│   └── resources.qrc
│
├── DesignerApp/                 # Designer Application (Legacy)
├── RuntimeApp/                  # Runtime Application (Legacy)
│
├── ExternalSystems/             # Subsystem health simulators
│   ├── external_system.py       # Single component monitor (TCP/UDP)
│   └── run_multiple_systems.py  # Multi-monitor launcher
│
├── assets/subsystems/           # Component images
│   ├── antenna/
│   ├── power_system/
│   ├── liquid_cooling_unit/
│   ├── communication_system/
│   └── radar_computer/
│
├── build_all.sh
├── clean_all.sh
└── README.md
```

## Features

### Modular Architecture
- Component types defined in JSON config, not code
- Add new types via UI dialog or config file
- Auto-populated component list from registry
- Data-driven rendering (image + fallback geometric shapes)
- String-based type system (no hardcoded enums)
- Full backward compatibility with existing .design files

### Multi-Protocol Support
- TCP (default) for reliable health data delivery
- UDP for low-latency health updates
- WebSocket and MQTT protocol options in registry
- Extensible protocol handler architecture

### Modern UI Design
- Glass-morphism inspired dark theme
- Teal/cyan accent color palette
- Rounded corners, subtle borders, gradient backgrounds
- Rich HTML analytics display with health bars
- Responsive card-based layout

### Designer Features
- Drag-and-drop component placement
- Dynamic component type list
- "+" button to add new types without code changes
- Save/Load system layouts
- Component analytics

### Runtime Features
- Load layouts from Designer
- Multi-protocol health server (TCP + UDP)
- Real-time component health updates
- Multi-client support
- Color-coded health status
- Comprehensive health analytics

## Extending the System

### Add New Component Types (No Code Changes!)

See "Adding a New Component Type" section above. Just edit `components.json` or use the UI.

### Add New Protocols

1. Extend `MessageServer` with a new receiver method
2. Add the protocol name to `ComponentRegistry::availableProtocols()`
3. That's it - existing components can select the new protocol

### Custom Health Monitors

Create your own monitor by sending JSON to port 12345 (TCP) or 12346 (UDP):

```python
import socket, json, time

# TCP example
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 12345))
msg = {"component_id": "component_1", "color": "#00FF00", "size": 95.0}
sock.sendall((json.dumps(msg) + '\n').encode())

# UDP example
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
msg = {"component_id": "component_1", "color": "#FFFF00", "size": 75.0}
sock.sendto((json.dumps(msg) + '\n').encode(), ('localhost', 12346))
```

## Troubleshooting

### Build Issues

**Problem**: `qmake: command not found`
```bash
sudo apt-get install qt5-qmake qtbase5-dev qtbase5-dev-tools libqt5network5
```

### Runtime Issues

**Problem**: Server fails to start
- Check if port 12345 is in use: `lsof -i :12345`

**Problem**: Components not updating
- Verify component IDs match (e.g., `component_1`, not `antenna_1`)
- Check the design file for actual component IDs
- Ensure messages are valid JSON with `component_id`, `color`, and `size` fields

**Problem**: New component type not showing
- Verify `components.json` is valid JSON
- Check the application console for registry loading messages
- Restart the application after editing `components.json`

## License

This project is provided as-is for educational and demonstration purposes.

## Author

Created as a Qt-based Radar System Monitoring demonstration with modular component architecture.
