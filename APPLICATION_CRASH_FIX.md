# Application Crash Fix - SubComponent Connection Cleanup

## Problem
The application was crashing when SubComponents were removed from a Component. This occurred in two scenarios:
1. When editing a component and removing subsystems via EditComponentDialog
2. When importing a component that replaces existing SubComponents

## Root Cause
When a SubComponent was deleted, the Connection objects that referenced it were not cleaned up first. This left dangling pointers in the Connection objects:
- `Connection::m_sourceItem` or `Connection::m_targetItem` pointed to freed memory
- When the scene tried to paint these connections (which happens automatically during rendering), the `paint()` method called `updatePosition()`
- `updatePosition()` accessed the deleted SubComponent through `getItemCenter()` and `getConnectionPoint()`
- Accessing freed memory caused a **segmentation fault crash**

### Code Locations Where Bug Occurred
1. **editcomponentdialog.cpp line 587**: `comp->removeSubComponent(0)` was called in a loop without cleaning up connections
2. **canvas.cpp line 1090**: Same issue when importing components and replacing SubComponents

## Solution

### 1. Added Connection Cleanup Methods to Canvas
Added two new methods to the Canvas class to handle connection cleanup:

**canvas.h:**
```cpp
void removeConnectionsInvolvingItem(QGraphicsItem* item);
void removeConnectionsInvolvingSubComponent(SubComponent* sub);
```

**canvas.cpp:**
These methods iterate through all connections and remove any that involve the specified item/SubComponent before the item is deleted.

### 2. Updated EditComponentDialog to Accept Canvas Parameter
Modified the EditComponentDialog constructor to accept a Canvas pointer:

**editcomponentdialog.h:**
```cpp
explicit EditComponentDialog(Component* component, Canvas* canvas = nullptr, QWidget* parent = nullptr);
```

**editcomponentdialog.cpp:**
Before removing each SubComponent, the dialog now:
1. Gets the SubComponent pointer
2. Calls `canvas->removeConnectionsInvolvingSubComponent(sub)` if canvas is available
3. Then removes the SubComponent

### 3. Updated Component to Pass Canvas to EditComponentDialog
**component.cpp:**
Modified the context menu handler to pass the canvas pointer to the EditComponentDialog:
```cpp
EditComponentDialog dialog(this, canvas);
```

### 4. Updated Canvas Import Method
**canvas.cpp:**
When importing a component, before clearing existing SubComponents:
1. Gets the SubComponent pointer
2. Calls `removeConnectionsInvolvingSubComponent(sub)`
3. Then removes the SubComponent

## Changes Made

### Files Modified:
1. **UnifiedApp/canvas.h** - Added new connection cleanup method declarations
2. **UnifiedApp/canvas.cpp** - Implemented connection cleanup methods and updated import logic
3. **UnifiedApp/editcomponentdialog.h** - Added Canvas parameter to constructor
4. **UnifiedApp/editcomponentdialog.cpp** - Updated to clean up connections before removing SubComponents
5. **UnifiedApp/component.cpp** - Updated to pass Canvas pointer to EditComponentDialog

## Testing
✅ Application should no longer crash when:
   - Editing a component and removing subsystems
   - Importing a component that replaces existing SubComponents
   - Connections involving SubComponents are properly cleaned up before SubComponent deletion

## Impact
- **Fixed**: Crash when removing SubComponents with active connections
- **Fixed**: Memory safety issue with dangling pointers in Connection objects
- **Improved**: Connection lifecycle management is now properly integrated with SubComponent removal
- **Maintained**: All existing functionality remains intact

## Technical Details
The fix ensures the following order of operations:
1. Identify all connections involving the SubComponent to be deleted
2. Remove those connections from the scene and delete them
3. Only then delete the SubComponent

This prevents the Connection objects from ever accessing deleted SubComponent memory.

## Commit
Branch: cursor/application-crash-4102
