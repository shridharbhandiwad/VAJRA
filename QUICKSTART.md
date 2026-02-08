# Quick Start Guide

Get up and running with the Component Editor and Analytics System in 5 minutes!

## Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install qt5-qmake qtbase5-dev qtbase5-dev-tools libqt5network5 python3

# Fedora
sudo dnf install qt5-qtbase-devel python3

# macOS
brew install qt@5 python3
```

## Step 1: Build (2 minutes)

```bash
# Build both applications
./build_all.sh
```

## Step 2: Create a Design (1 minute)

```bash
# Start Designer Application
./DesignerApp/DesignerApp
```

1. Drag a **Circle** to the canvas (left side)
2. Drag a **Square** to the canvas (middle)
3. Drag a **Triangle** to the canvas (right side)
4. Click **"Save Design"**
5. Save as `test.design`
6. Close the Designer Application

## Step 3: Run Runtime Application (30 seconds)

```bash
# Start Runtime Application
./RuntimeApp/RuntimeApp
```

1. Click **"Load Design"**
2. Select `test.design`
3. You should see your three components
4. Keep this window open

## Step 4: Start External Systems (30 seconds)

```bash
# In a new terminal
cd ExternalSystems
python3 run_multiple_systems.py
```

## Step 5: Watch the Magic! (1 minute)

- **Canvas**: Watch components change colors and sizes in real-time
- **Analytics**: See statistics update as messages arrive
- **Status Bar**: Shows connected clients (should be 3)

Press `Ctrl+C` in the terminal to stop external systems.

## What's Happening?

1. **Designer App** created a layout with 3 components (IDs: component_1, component_2, component_3)
2. **Runtime App** loaded the layout and started a TCP server on port 12345
3. **External Systems** connected to the server and send periodic messages
4. Each message contains a random color and size
5. Runtime App updates components and tracks analytics

## Next Steps

### Customize Update Speed

```bash
# Slower updates (5 seconds)
python3 run_multiple_systems.py --interval 5.0

# Faster updates (0.5 seconds)  
python3 run_multiple_systems.py --interval 0.5
```

### Run Individual Systems

```bash
# Control just one component
python3 external_system.py component_1 --interval 1.0
```

### Create Custom Designs

1. Open Designer App
2. Create any layout you want
3. Note the component IDs in the Analytics panel
4. Save your design
5. Load it in Runtime App
6. Start external systems with matching component IDs:

```bash
python3 run_multiple_systems.py --components component_1 component_2 component_3 component_4
```

## Troubleshooting

### Build fails with "qmake not found"
```bash
# Install Qt development tools (see Prerequisites above)
```

### External systems can't connect
```bash
# Make sure Runtime App is running and has loaded a design
# Check the status bar shows "Server Status: Running on port 12345"
```

### Components don't update
```bash
# Verify component IDs match:
# - Designer App shows IDs in Analytics panel
# - External systems must use the same IDs
```

## Tips

- **Component IDs** start from component_1 and increment automatically
- **Default colors** are blue, but will change immediately when external systems connect
- **Size range** is typically 30-100, but can be customized with `--size-min` and `--size-max`
- **Multiple clients** are supported - run as many external systems as you want!

## System Architecture

```
Designer App → Save Design → Runtime App ← External Systems
                 (.design)      (TCP :12345)    (JSON messages)
```

## Example Workflow

```bash
# Terminal 1: Build
./build_all.sh

# Terminal 2: Design (GUI)
./DesignerApp/DesignerApp
# Create layout, save as test.design, close

# Terminal 2: Runtime (GUI)
./RuntimeApp/RuntimeApp  
# Load test.design, wait for connections

# Terminal 3: Simulators
cd ExternalSystems
python3 run_multiple_systems.py
# Watch the components animate!
```

Enjoy! 🎨
