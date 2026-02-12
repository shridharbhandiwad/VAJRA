# Imported Components Auto-Registration Feature

## Overview
When you import any component using the "IMPORT COMPONENT" button in Designer Mode, the component type will now automatically be registered in the ComponentRegistry and appear in the components lab (component list panel).

## What Changed

### 1. Enhanced Component Export (editcomponentdialog.cpp)
- Component exports now include the full component type definition from the ComponentRegistry
- The exported `.cmp` file contains a new `componentDefinition` field with all metadata:
  - Type ID, display name, label
  - Description and image directory
  - Icon color and shape
  - Subsystem list
  - Protocol and port information
  - Category
  - Allowed widgets

### 2. Enhanced Component Import (canvas.cpp)
- Import now checks if the component type exists in the registry
- If the type doesn't exist AND the import file contains a `componentDefinition`:
  - Automatically registers the new component type
  - Saves the registry to persist changes
  - The component type immediately appears in the components lab
- If the type doesn't exist and no definition is provided, import fails with error message

## How It Works

### Export Workflow
1. User right-clicks a component in Designer Mode
2. Selects "Edit Component"
3. Clicks "💾 Export Component (.cmp)"
4. System exports:
   - Component instance data (position, color, size, etc.)
   - Component type definition from registry
   - Subsystems and design widgets

### Import Workflow
1. User clicks "IMPORT COMPONENT" button in Designer Mode
2. Selects a `.cmp` file
3. System checks if component type exists:
   - **If exists**: Creates component instance on canvas
   - **If doesn't exist**: 
     - Reads `componentDefinition` from file
     - Registers new component type
     - Saves registry
     - Creates component instance on canvas
     - **New type appears in components lab!**

## Benefits

1. **Seamless Sharing**: Share custom component types between designers
2. **No Manual Registration**: No need to manually add component types via "Add Component Type" dialog
3. **Backward Compatible**: Still works with old `.cmp` files (but type must exist in registry)
4. **Automatic Persistence**: Registry is automatically saved after successful import

## Example Exported Component File

```json
{
  "type": "component",
  "version": "1.0",
  "typeId": "CustomRadar",
  "id": "component_1",
  "displayName": "Custom Radar",
  "label": "RAD",
  "color": "#4CAF50",
  "size": 100.0,
  "userWidth": 200,
  "userHeight": 150,
  "x": 400,
  "y": 300,
  "componentDefinition": {
    "type_id": "CustomRadar",
    "display_name": "Custom Radar",
    "label": "RAD",
    "description": "Custom radar system with advanced features",
    "image_dir": "custom_radar",
    "icon_color": "#4CAF50",
    "subsystems": [
      "Signal Processor",
      "Antenna Array",
      "Power Supply"
    ],
    "protocol": "TCP",
    "port": 12345,
    "category": "Sensor",
    "shape": "ellipse",
    "allowed_widgets": ["Label", "LineEdit", "Button"]
  },
  "subsystems": [...],
  "designWidgets": [...]
}
```

## Testing the Feature

### Test Case 1: Import Custom Component Type
1. Launch UnifiedApp as Designer
2. Create a custom component type (or use existing)
3. Add component to canvas
4. Right-click → Edit Component → Export Component
5. Close and restart application
6. Delete the custom component type from registry
7. Click "IMPORT COMPONENT"
8. Select the exported `.cmp` file
9. **Expected Result**: 
   - Component appears on canvas
   - Component type appears in components lab
   - Can drag new instances from component list

### Test Case 2: Import with Existing Type
1. Launch UnifiedApp as Designer
2. Export a component of existing type (e.g., Antenna)
3. Import the file
4. **Expected Result**:
   - Component appears on canvas
   - No duplicate type in component list
   - Existing type remains unchanged

### Test Case 3: Share Between Designers
1. Designer A creates custom component type "GPS Tracker"
2. Designer A exports an instance
3. Designer A shares `.cmp` file with Designer B
4. Designer B imports the file
5. **Expected Result**:
   - Designer B can use "GPS Tracker" type
   - Type appears in Designer B's component lab
   - Designer B can create new instances

## Files Modified

1. `UnifiedApp/editcomponentdialog.cpp`
   - Modified `onExportComponent()` function
   - Added component definition export

2. `UnifiedApp/canvas.cpp`
   - Modified `importComponent()` function
   - Added automatic component type registration
   - Added registry persistence

## Technical Notes

- Uses existing `ComponentDefinition::toJson()` and `ComponentDefinition::fromJson()` methods
- Maintains backward compatibility with old export files
- Automatic registry save ensures persistence across sessions
- ComponentList automatically refreshes via signal/slot connection when registry changes
- No UI changes required - feature works seamlessly with existing interface

## Future Enhancements

Potential improvements for future iterations:
1. Conflict resolution UI when importing type that conflicts with existing type
2. Bulk import multiple component files
3. Import/export entire component library as single file
4. Version checking for component definitions
5. Import statistics/summary dialog
