# Designer Application

Visual editor for creating component layouts that can be loaded by the Runtime Application.

## Features

- Drag-and-drop interface for component placement
- Three component types: Circle, Square, Triangle
- Visual canvas with component positioning
- Analytics panel showing component statistics
- Save/Load design files (.design format)

## Building

```bash
cd DesignerApp
qmake DesignerApp.pro
make
```

## Running

```bash
./DesignerApp
```

## Usage

1. **Add Components**: Drag components from the "All Components" list to the canvas
2. **Position Components**: Click and drag components on the canvas to reposition them
3. **View Analytics**: The right panel shows statistics about components on the canvas
4. **Save Design**: Click "Save Design" to export your layout to a .design file
5. **Load Design**: Click "Load Design" to import a previously saved design
6. **Clear Canvas**: Click "Clear Canvas" to remove all components

## Component Types

- **Circle**: Round component
- **Square**: Rectangular component  
- **Triangle**: Triangular component

## Design File Format

Design files use JSON format:

```json
{
  "components": [
    {
      "id": "component_1",
      "type": "Circle",
      "x": 200.0,
      "y": 150.0,
      "color": "#0000ff",
      "size": 50.0
    }
  ]
}
```

## Tips

- Component IDs are automatically generated (component_1, component_2, etc.)
- Note the component IDs when designing - you'll need them for external systems
- Position components with enough space for size changes in the runtime app
- Default component size is 50, but external systems can change this to 30-100+
