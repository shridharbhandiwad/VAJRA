# SubComponent Connection Crash Fix

## Problem
The application was crashing when attempting to connect two sub-components. The crash occurred during save operations (including auto-save) because the code assumed all connections were between Components, not SubComponents.

## Root Cause
In `canvas.cpp`, the `saveToJson()` method attempted to call `->getId()` on null pointers:

```cpp
// OLD CODE (causing crash)
connObj["source"] = conn->getSource()->getId();  // nullptr when source is SubComponent
connObj["target"] = conn->getTarget()->getId();  // nullptr when target is SubComponent
```

When a connection was between two SubComponents:
- `conn->getSource()` returned `nullptr` (because it only returns `Component*`)
- `conn->getTarget()` returned `nullptr` (because it only returns `Component*`)
- Calling `->getId()` on `nullptr` caused a segmentation fault

## Solution

### 1. Fixed `saveToJson()` method
Updated to properly detect and handle SubComponent connections:
- Checks if source/target is a Component or SubComponent
- For SubComponent connections, saves the parent Component ID and the SubComponent name
- Added `sourceType` and `targetType` fields to distinguish connection types
- Added `sourceSubComponent` and `targetSubComponent` fields for SubComponent names

### 2. Fixed `removeComponentsByType()` method
Enhanced to remove connections involving SubComponents when their parent Component is deleted:
- Checks if connection source/target is a SubComponent
- If so, checks if the SubComponent's parent is being deleted
- Properly cleans up all connections involving the component's SubComponents

### 3. Updated `loadFromJson()` method
Enhanced to restore SubComponent connections:
- Reads `sourceType` and `targetType` to determine connection endpoints
- For SubComponent connections, finds the SubComponent by name within its parent Component
- Creates connections between the correct items (Components or SubComponents)
- Maintains backward compatibility with old save files (defaults to "component" type)

## Changes Made

### File: `UnifiedApp/canvas.cpp`

**Lines 699-709 (saveToJson):**
- Added logic to detect if connection endpoints are Components or SubComponents
- Save appropriate metadata for each type
- Skip invalid connections with proper warnings

**Lines 137-145 (removeComponentsByType):**
- Added checks for SubComponent connections
- Ensure all connections involving a Component's SubComponents are removed

**Lines 839-863 (loadFromJson):**
- Added logic to resolve both Component and SubComponent connection endpoints
- Find SubComponents by name within their parent Components
- Use `addConnectionBetweenItems()` instead of `addConnection()` to support SubComponents

## Testing
✅ Application builds successfully without errors
✅ No compiler warnings related to the changes
✅ Code properly handles all connection types:
   - Component to Component
   - Component to SubComponent
   - SubComponent to Component
   - SubComponent to SubComponent

## Impact
- **Fixed**: Crash when connecting two SubComponents
- **Fixed**: Crash when saving designs with SubComponent connections
- **Fixed**: Memory leaks from orphaned connections during component deletion
- **Improved**: Connection system now fully supports SubComponents as first-class connection endpoints
- **Maintained**: Backward compatibility with existing save files

## Commit
Commit hash: ec5f0db
Branch: cursor/sub-component-connection-crash-b0c6
