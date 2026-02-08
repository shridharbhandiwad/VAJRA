# Component Editor and Analytics System

A comprehensive Qt-based application system for designing, deploying, and monitoring dynamic component-based applications. This system consists of three main parts:

1. **Designer Application** - Visual editor for creating component layouts
2. **Runtime Application** - Displays components and receives real-time updates from external systems
3. **External System Simulators** - Python scripts that simulate external systems sending update messages

## System Overview

### Designer Application

The Designer Application provides a visual interface for creating component layouts with:
- **All Components Panel** - Draggable component list (Circle, Square, Triangle)
- **Designer View** - Canvas for placing and arranging components
- **Analytics Panel** - Statistics about components on the canvas
- **Save/Load** - Export designs to `.design` files

### Runtime Application

The Runtime Application is the production application that:
- Loads designs created by the Designer Application
- Receives messages from external systems via TCP socket (port 12345)
- Updates component colors and sizes based on received messages
- Tracks analytics: message counts, color changes, size changes
- Does NOT include the component list panel (canvas and analytics only)

### External Systems

Python simulators that:
- Connect to the Runtime Application via TCP
- Send periodic JSON messages to update specific components
- Each external system controls one component (by ID)
- Messages include: component_id, color, size

## Architecture

```
┌─────────────────────┐
│ Designer App        │
│                     │
│  ┌──────────────┐   │
│  │ Components   │   │
│  │ - Circle     │   │
│  │ - Square     │   │
│  │ - Triangle   │   │
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
│ Runtime App         │◄──────│ External Systems    │
│                     │ TCP   │                     │
│  ┌──────────────┐   │:12345 │  ┌──────────────┐   │
│  │ Canvas       │◄──┼───────┤  │ System 1     │   │
│  │ (display)    │   │       │  │ (component_1)│   │
│  └──────────────┘   │       │  └──────────────┘   │
│                     │       │                     │
│  ┌──────────────┐   │       │  ┌──────────────┐   │
│  │ Analytics    │   │       │  │ System 2     │   │
│  │ (real-time)  │   │       │  │ (component_2)│   │
│  └──────────────┘   │       │  └──────────────┘   │
│                     │       │                     │
│  [Load Design]      │       │  ┌──────────────┐   │
└─────────────────────┘       │  │ System 3     │   │
                              │  │ (component_3)│   │
                              │  └──────────────┘   │
                              └─────────────────────┘
```

## Requirements

### For Qt Applications
- Qt 5.x or Qt 6.x
- C++ compiler (g++, clang, or MSVC)
- Qt development tools (qmake, Qt Creator optional)

### For External Systems
- Python 3.x (standard library only, no external packages needed)

## Building

### Linux/macOS

```bash
# Make scripts executable
chmod +x build_all.sh clean_all.sh

# Build all applications
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

#### Step 1: Design Your Layout

```bash
# Run Designer Application
./DesignerApp/DesignerApp
```

1. Drag components (Circle, Square, Triangle) from the left panel to the canvas
2. Position components as desired
3. Note the component IDs (e.g., component_1, component_2)
4. Click "Save Design" and save as `mydesign.design`

#### Step 2: Run Runtime Application

```bash
# Run Runtime Application
./RuntimeApp/RuntimeApp
```

1. Click "Load Design"
2. Select your saved `.design` file
3. The canvas will display your components
4. The server will start listening on port 12345

#### Step 3: Start External Systems

```bash
cd ExternalSystems

# Option 1: Run individual systems
python3 external_system.py component_1 &
python3 external_system.py component_2 &
python3 external_system.py component_3 &

# Option 2: Run multiple systems at once
python3 run_multiple_systems.py --components component_1 component_2 component_3
```

#### Step 4: Watch Real-Time Updates

- Components will change color and size based on messages from external systems
- The Analytics panel shows statistics:
  - Number of messages received per component
  - Current color and size
  - Number of color changes
  - Number of size changes
  - Total messages across all components

## Message Protocol

External systems communicate with the Runtime Application via TCP socket using JSON messages:

### Message Format

```json
{
  "component_id": "component_1",
  "color": "#FF0000",
  "size": 75.5
}
```

### Fields
- `component_id` (string): ID of the component to update
- `color` (string): Hex color code (e.g., "#FF0000" for red)
- `size` (number): Component size (recommended: 30-100)

### Connection
- **Protocol**: TCP
- **Port**: 12345
- **Format**: JSON, one message per line (newline-delimited)

## Examples

### Create a Simple Design

1. Start Designer Application
2. Drag one Circle, one Square, and one Triangle to canvas
3. Arrange them in a row
4. Save as `simple.design`

### Test with External Systems

```bash
# Terminal 1: Start Runtime App
./RuntimeApp/RuntimeApp
# Load simple.design

# Terminal 2: Start simulators
cd ExternalSystems
python3 run_multiple_systems.py
```

Watch the components animate with changing colors and sizes!

### Custom External System

Create your own external system by connecting to port 12345 and sending JSON messages:

```python
import socket
import json
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 12345))

while True:
    message = {
        "component_id": "component_1",
        "color": "#00FF00",
        "size": 60.0
    }
    sock.sendall((json.dumps(message) + '\n').encode('utf-8'))
    time.sleep(2)
```

## Project Structure

```
.
├── DesignerApp/              # Designer Application
│   ├── DesignerApp.pro       # Qt project file
│   ├── main.cpp
│   ├── mainwindow.cpp/h      # Main window with UI
│   ├── component.cpp/h       # Component graphics item
│   ├── canvas.cpp/h          # Canvas widget
│   ├── componentlist.cpp/h   # Component list with drag/drop
│   └── analytics.cpp/h       # Analytics widget
│
├── RuntimeApp/               # Runtime Application
│   ├── RuntimeApp.pro        # Qt project file
│   ├── main.cpp
│   ├── mainwindow.cpp/h      # Main window with UI
│   ├── component.cpp/h       # Component graphics item
│   ├── canvas.cpp/h          # Canvas widget
│   ├── analytics.cpp/h       # Analytics widget
│   └── messageserver.cpp/h   # TCP server for messages
│
├── ExternalSystems/          # External system simulators
│   ├── external_system.py    # Single system simulator
│   ├── run_multiple_systems.py  # Multi-system launcher
│   └── README.md
│
├── build_all.sh              # Build script (Linux/macOS)
├── clean_all.sh              # Clean script (Linux/macOS)
└── README.md                 # This file
```

## Features

### Designer Application Features
- ✓ Drag-and-drop component placement
- ✓ Three component types: Circle, Square, Triangle
- ✓ Save/Load designs
- ✓ Component analytics
- ✓ Visual component selection

### Runtime Application Features
- ✓ Load designs from Designer App
- ✓ TCP server for external communication
- ✓ Real-time component updates
- ✓ Multi-client support
- ✓ Comprehensive analytics tracking
- ✓ Connection status monitoring

### External System Features
- ✓ Random color generation from palette
- ✓ Random size generation within range
- ✓ Configurable update intervals
- ✓ Command-line configuration
- ✓ Multi-system manager
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

**Problem**: External systems can't connect
- Ensure Runtime Application is running and loaded a design
- Check firewall settings
- Verify correct host and port

**Problem**: Components not updating
- Verify component IDs match between design and external systems
- Check external system console for connection status
- Ensure messages are properly formatted JSON

## Extending the System

### Add New Component Types

1. Update `ComponentType` enum in `component.h`
2. Add rendering code in `Component::paint()`
3. Add to component list in `componentlist.cpp`
4. Update type name conversions

### Change Communication Protocol

Modify `messageserver.cpp` to support different protocols:
- UDP instead of TCP
- WebSocket for web integration
- MQTT for IoT scenarios
- gRPC for microservices

### Add More Analytics

Enhance `analytics.cpp` to track:
- Average size over time
- Color histogram
- Update frequency per component
- Time-series graphs

## License

This project is provided as-is for educational and demonstration purposes.

## Author

Created as a Qt application demonstration system.
