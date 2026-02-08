# External Systems Simulators

These Python scripts simulate external systems that send periodic messages to the Runtime Application to update component colors and sizes.

## Files

- `external_system.py` - Single external system simulator
- `run_multiple_systems.py` - Launches multiple external systems simultaneously

## Requirements

- Python 3.x (no additional packages required, uses only standard library)

## Usage

### Single External System

Run a single external system for a specific component:

```bash
python3 external_system.py component_1
```

Options:
- `--host` - Server host (default: localhost)
- `--port` - Server port (default: 12345)
- `--interval` - Update interval in seconds (default: 2.0)
- `--size-min` - Minimum component size (default: 30)
- `--size-max` - Maximum component size (default: 100)

Example:
```bash
python3 external_system.py component_1 --interval 1.5 --size-min 40 --size-max 120
```

### Multiple External Systems

Run multiple external systems at once:

```bash
python3 run_multiple_systems.py
```

This will start external systems for component_1, component_2, and component_3 by default.

Options:
- `--components` - List of component IDs (default: component_1 component_2 component_3)
- `--host` - Server host (default: localhost)
- `--port` - Server port (default: 12345)
- `--interval` - Base update interval in seconds (default: 2.0)

Example:
```bash
python3 run_multiple_systems.py --components component_1 component_2 component_3 component_4 --interval 3.0
```

## Message Format

External systems send JSON messages in the following format:

```json
{
  "component_id": "component_1",
  "color": "#FF0000",
  "size": 75.5
}
```

## How It Works

1. Each external system connects to the Runtime Application via TCP socket
2. Periodically sends update messages with random colors and sizes
3. The Runtime Application updates the component appearance based on received messages
4. Analytics panel tracks all received messages and changes

## Testing

1. Start the Runtime Application
2. Load a design with components (e.g., component_1, component_2, component_3)
3. Run the external systems with matching component IDs
4. Watch the components change color and size in real-time
5. Monitor the Analytics panel to see statistics
