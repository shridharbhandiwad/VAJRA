# Quick Start Guide - Unified Radar System Application

## 1. Build the Application

```bash
cd UnifiedApp
qmake UnifiedApp.pro
make
```

## 2. Launch the Application

```bash
./UnifiedApp
```

## 3. Login

When the login dialog appears, choose your role:

### Option A: Commander Mode
```
Username: Commander
Password: designer
```

**What you can do:**
- Create new radar system layouts
- Drag and drop components onto the canvas
- Save designs to files
- Load and edit existing designs
- View component analytics

### Option B: Operator Mode (Runtime Monitoring)
```
Username: Operator
Password: user
```

**What you can do:**
- Load existing radar system layouts
- Monitor real-time health status
- View messages from external systems
- Track component statistics

## 4. Basic Workflow

### Commander Workflow
1. Login with Commander credentials
2. Drag components from the left panel onto the canvas
3. Arrange components as needed
4. Click "Save Design" to save your layout
5. Use "Load Design" to open existing layouts
6. Use "Clear Canvas" to start fresh

### Operator Workflow (Runtime Monitoring)
1. Login with Operator credentials
2. Click "Load Design" to open a radar system layout
3. Start external system simulators (see `ExternalSystems/` folder)
4. Watch as components change color based on health status
5. Monitor connection count in the status bar
6. View message history in the analytics panel

## 5. Testing with External Systems

From another terminal:

```bash
cd ExternalSystems
python3 external_system.py
```

This will send simulated health status updates to the UnifiedApp when running in Operator mode.

## Tips

- **Commander mode** is for system architects and engineers who design layouts
- **Operator mode** is for operators who monitor system health
- Both modes share the same design files (`.design` format)
- The server automatically starts on port 12345 in Operator mode
- You can run multiple external system simulators simultaneously

## Example Scenario

1. Login as Commander
2. Create a radar system with 5 components
3. Save as `my_radar_system.design`
4. Logout and login as Operator
5. Load `my_radar_system.design`
6. Run external system simulator
7. Watch components update with health status

That's it! You're ready to use the Unified Radar System Application.
