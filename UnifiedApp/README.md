# UnifiedApp - Modular Radar System Monitor (v3.0)

The recommended unified application combining Designer and Runtime capabilities with a **fully modular component architecture**.

## Key Feature: Zero-Code Component Extension

New component types can be added **without modifying any C++ code**:

1. **Via the UI**: Click "+" Add Component Type" button in Designer mode
2. **Via JSON**: Edit `components.json` configuration file
3. **Image support**: Place images in `assets/subsystems/<dir>/`

## Architecture

```
components.json ──→ ComponentRegistry (singleton)
                         │
         ┌───────────────┼───────────────┐
         ▼               ▼               ▼
   ComponentList      Canvas          Analytics
   (auto-populates)  (drag/drop)    (dynamic types)
         │               │
         ▼               ▼
   AddComponentDialog  Component
   (runtime type add)  (data-driven rendering)
```

### Core Classes

| Class | Purpose |
|-------|---------|
| `ComponentRegistry` | Singleton that loads/saves/manages component definitions from JSON |
| `ComponentDefinition` | Data struct for one component type (name, image, subsystems, protocol, etc.) |
| `Component` | Graphics item that renders based on registry data, no hardcoded types |
| `ComponentList` | Auto-populates from registry, refreshes when types change |
| `Canvas` | Resolves drag-drop types via registry, full backward compatibility |
| `AddComponentDialog` | UI for defining new types at runtime |
| `MessageServer` | Multi-protocol receiver (TCP port 12345, UDP port 12346) |
| `Analytics` | Dynamic HTML display supporting any component type |

## Quick Start

```bash
cd UnifiedApp
qmake UnifiedApp.pro
make
./UnifiedApp
```

### Login Credentials

| Username | Password | Role |
|----------|----------|------|
| Designer | designer | Full design capabilities + add component types |
| User | user | Runtime health monitoring |

## Adding New Component Types

### Method 1: UI Dialog

1. Login as Designer
2. Click **"+ ADD COMPONENT TYPE"** in toolbar (or "+" New Type" in side panel)
3. Fill in the form:
   - **Component Name**: e.g., "GPS Receiver"
   - **Short Label**: e.g., "GPS"
   - **Description**: Brief description
   - **Image Directory**: Directory name under `assets/subsystems/`
   - **Subsystems**: One per line (e.g., "Signal Strength", "Satellite Lock")
   - **Protocol**: TCP, UDP, WebSocket, or MQTT
   - **Port**: Default 12345 for TCP, 12346 for UDP
   - **Category**: Sensor, Infrastructure, Network, Processing, Navigation, etc.
   - **Fallback Shape**: rect, ellipse, hexagon, or diamond
4. Click "ADD COMPONENT" - immediately available in the component list!

### Method 2: Edit components.json

Add a new entry to the `components` array:

```json
{
  "type_id": "GpsReceiver",
  "display_name": "GPS Receiver",
  "label": "GPS",
  "description": "Satellite navigation and positioning system",
  "image_dir": "gps_receiver",
  "icon_color": "#2196F3",
  "subsystems": ["Signal Strength", "Satellite Lock Count", "Position Accuracy"],
  "protocol": "UDP",
  "port": 12346,
  "category": "Navigation",
  "shape": "hexagon"
}
```

### Method 3: With Custom Image

1. Create `assets/subsystems/gps_receiver/` directory
2. Add `gps_receiver_main.jpg` (or `.png`) image file
3. Add the JSON definition as above with `"image_dir": "gps_receiver"`
4. The component will display your custom image on the canvas

## Component Definition Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `type_id` | string | Yes | Unique identifier (PascalCase, e.g., "GpsReceiver") |
| `display_name` | string | Yes | Human-readable name (e.g., "GPS Receiver") |
| `label` | string | Yes | Short canvas label (max 5 chars, e.g., "GPS") |
| `description` | string | No | Brief description of the component |
| `image_dir` | string | No | Directory name under `assets/subsystems/` |
| `icon_color` | string | No | Hex color for fallback rendering (default: blue) |
| `subsystems` | array | No | List of sub-component names for health tracking |
| `protocol` | string | No | "TCP", "UDP", "WebSocket", or "MQTT" (default: "TCP") |
| `port` | int | No | Port number (default: 12345) |
| `category` | string | No | Grouping category (default: "General") |
| `shape` | string | No | Fallback shape: "rect", "ellipse", "hexagon", "diamond" |

## Multi-Protocol Support

The MessageServer supports receiving health data via multiple protocols:

| Protocol | Port | Format | Status |
|----------|------|--------|--------|
| TCP | 12345 | Line-delimited JSON | Fully implemented |
| UDP | 12346 | JSON datagrams | Fully implemented |
| WebSocket | - | JSON | Extensible (registry option) |
| MQTT | - | JSON topics | Extensible (registry option) |

## Design File Format

Design files (`.design`) store component placements:

```json
{
  "components": [
    {
      "id": "component_1",
      "type": "Antenna",
      "x": 300,
      "y": 100,
      "color": "#4CAF50",
      "size": 50
    }
  ]
}
```

**Backward compatible**: Files using old type names (e.g., "PowerSystem") are automatically resolved via the registry.

## UI Design

Modern glass-morphism inspired dark theme with:
- Teal/cyan (#00BCD4) accent colors
- Rounded corners (8-12px)
- Semi-transparent panel backgrounds
- Gradient buttons
- Rich HTML analytics with health bars
- Responsive card-based layout
