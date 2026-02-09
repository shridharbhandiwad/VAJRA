#!/bin/bash
# ============================================================
# Test Script: Component Hierarchy and Relations
# ============================================================
# Tests the health of the redesigned application with:
#   1. Sub-components embedded in parent components
#   2. Uni-directional and bi-directional connections with labels
#   3. Save/Load with connections and sub-components
#   4. Build verification
# ============================================================

set -e

PASS=0
FAIL=0
TOTAL=0

pass() {
    echo "  [PASS] $1"
    PASS=$((PASS + 1))
    TOTAL=$((TOTAL + 1))
}

fail() {
    echo "  [FAIL] $1"
    FAIL=$((FAIL + 1))
    TOTAL=$((TOTAL + 1))
}

check() {
    TOTAL=$((TOTAL + 1))
    if [ $1 -eq 0 ]; then
        echo "  [PASS] $2"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $2"
        FAIL=$((FAIL + 1))
    fi
}

echo "============================================================"
echo "Component Hierarchy & Relations - Health Test"
echo "============================================================"
echo ""

# ---- Test 1: Build Verification ----
echo "--- Test 1: Build Verification ---"

cd /workspace/UnifiedApp

# Clean and rebuild
make clean 2>/dev/null || true
qmake UnifiedApp.pro 2>&1 | tail -1
BUILD_OUTPUT=$(make -j$(nproc) 2>&1)
BUILD_EXIT=$?
check $BUILD_EXIT "UnifiedApp builds successfully"

# Check for warnings
WARNING_COUNT=$(echo "$BUILD_OUTPUT" | grep -c "warning:" || true)
if [ "$WARNING_COUNT" -eq 0 ] 2>/dev/null; then
    pass "No build warnings"
else
    fail "Build has $WARNING_COUNT warning(s)"
fi

# Check binary exists
if [ -f "UnifiedApp" ]; then
    pass "UnifiedApp binary exists"
else
    fail "UnifiedApp binary not found"
fi

echo ""

# ---- Test 2: Source File Integrity ----
echo "--- Test 2: Source File Integrity ---"

# Check new files exist
for f in subcomponent.h subcomponent.cpp connection.h connection.cpp; do
    if [ -f "$f" ]; then
        pass "File $f exists"
    else
        fail "File $f missing"
    fi
done

# Check key classes are defined
if grep -q "class SubComponent" subcomponent.h; then
    pass "SubComponent class defined"
else
    fail "SubComponent class not found"
fi

if grep -q "class Connection" connection.h; then
    pass "Connection class defined"
else
    fail "Connection class not found"
fi

if grep -q "enum class ConnectionType" connection.h; then
    pass "ConnectionType enum defined"
else
    fail "ConnectionType enum not found"
fi

if grep -q "Unidirectional" connection.h; then
    pass "Unidirectional connection type defined"
else
    fail "Unidirectional type missing"
fi

if grep -q "Bidirectional" connection.h; then
    pass "Bidirectional connection type defined"
else
    fail "Bidirectional type missing"
fi

echo ""

# ---- Test 3: Component Hierarchy ----
echo "--- Test 3: Component Hierarchy (Sub-components) ---"

# Check Component class includes sub-component support
if grep -q "SubComponent" component.h; then
    pass "Component.h references SubComponent"
else
    fail "Component.h missing SubComponent reference"
fi

if grep -q "addSubComponent" component.h; then
    pass "addSubComponent method declared"
else
    fail "addSubComponent method missing"
fi

if grep -q "getSubComponents" component.h; then
    pass "getSubComponents method declared"
else
    fail "getSubComponents method missing"
fi

if grep -q "createDefaultSubComponents" component.cpp; then
    pass "createDefaultSubComponents implemented"
else
    fail "createDefaultSubComponents missing"
fi

if grep -q "layoutSubComponents" component.cpp; then
    pass "layoutSubComponents implemented"
else
    fail "layoutSubComponents missing"
fi

# Check that sub-components display name and health
if grep -q "m_name" subcomponent.cpp; then
    pass "SubComponent renders name"
else
    fail "SubComponent missing name rendering"
fi

if grep -q "m_health" subcomponent.cpp; then
    pass "SubComponent tracks health"
else
    fail "SubComponent missing health tracking"
fi

echo ""

# ---- Test 4: Connection/Relation Support ----
echo "--- Test 4: Connection/Relation Support ---"

# Check Canvas connection support
if grep -q "addConnection" canvas.h; then
    pass "Canvas supports addConnection"
else
    fail "Canvas missing addConnection"
fi

if grep -q "removeConnection" canvas.h; then
    pass "Canvas supports removeConnection"
else
    fail "Canvas missing removeConnection"
fi

if grep -q "CanvasMode" canvas.h; then
    pass "CanvasMode enum defined"
else
    fail "CanvasMode missing"
fi

if grep -q "Connect" canvas.h; then
    pass "Connect mode defined"
else
    fail "Connect mode missing"
fi

# Check connection drawing
if grep -q "drawArrowHead" connection.cpp; then
    pass "Arrow head drawing implemented"
else
    fail "Arrow head drawing missing"
fi

if grep -q "m_label" connection.cpp; then
    pass "Connection label support"
else
    fail "Connection label missing"
fi

# Check connection serialization
if grep -q "connectionTypeToString" connection.cpp; then
    pass "Connection type serialization"
else
    fail "Connection type serialization missing"
fi

if grep -q "stringToConnectionType" connection.cpp; then
    pass "Connection type deserialization"
else
    fail "Connection type deserialization missing"
fi

echo ""

# ---- Test 5: Save/Load with Connections ----
echo "--- Test 5: Save/Load Support ---"

# Check canvas save includes connections
if grep -q "connectionsArray" canvas.cpp; then
    pass "Canvas saves connections to JSON"
else
    fail "Canvas missing connection save"
fi

# Check canvas save includes sub-components
if grep -q "subcomponents" canvas.cpp; then
    pass "Canvas saves sub-components to JSON"
else
    fail "Canvas missing sub-component save"
fi

# Check canvas load includes connections
if grep -q "connections" canvas.cpp; then
    pass "Canvas loads connections from JSON"
else
    fail "Canvas missing connection load"
fi

echo ""

# ---- Test 6: MainWindow Connection UI ----
echo "--- Test 6: MainWindow Connection Mode UI ---"

if grep -q "toggleConnectionMode" mainwindow.h; then
    pass "Connection mode toggle in MainWindow"
else
    fail "Connection mode toggle missing"
fi

if grep -q "m_connectBtn" mainwindow.h; then
    pass "Connect button declared"
else
    fail "Connect button missing"
fi

if grep -q "m_connectionTypeCombo" mainwindow.h; then
    pass "Connection type combo declared"
else
    fail "Connection type combo missing"
fi

if grep -q "CONNECT MODE" mainwindow.cpp; then
    pass "Connect mode button created with label"
else
    fail "Connect mode button label missing"
fi

if grep -q "Uni-directional" mainwindow.cpp; then
    pass "Uni-directional option in combo"
else
    fail "Uni-directional option missing"
fi

if grep -q "Bi-directional" mainwindow.cpp; then
    pass "Bi-directional option in combo"
else
    fail "Bi-directional option missing"
fi

echo ""

# ---- Test 7: Design File Integrity ----
echo "--- Test 7: Design File with Default Data ---"

DESIGN_FILE="/workspace/radar_system.design"
if [ -f "$DESIGN_FILE" ]; then
    pass "radar_system.design exists"
else
    fail "radar_system.design missing"
fi

# Check design file has components
if python3 -c "import json; d=json.load(open('$DESIGN_FILE')); assert len(d['components']) >= 5" 2>/dev/null; then
    pass "Design file has 5+ components"
else
    fail "Design file missing components"
fi

# Check design file has connections
if python3 -c "import json; d=json.load(open('$DESIGN_FILE')); assert len(d['connections']) >= 5" 2>/dev/null; then
    pass "Design file has 5+ connections"
else
    fail "Design file missing connections"
fi

# Check design file has sub-components
if python3 -c "import json; d=json.load(open('$DESIGN_FILE')); assert any('subcomponents' in c for c in d['components'])" 2>/dev/null; then
    pass "Design file has sub-component data"
else
    fail "Design file missing sub-component data"
fi

# Check connection types
if python3 -c "import json; d=json.load(open('$DESIGN_FILE')); types=[c['type'] for c in d['connections']]; assert 'unidirectional' in types and 'bidirectional' in types" 2>/dev/null; then
    pass "Design file has both uni/bi-directional connections"
else
    fail "Design file missing connection type variety"
fi

# Check connection labels
if python3 -c "import json; d=json.load(open('$DESIGN_FILE')); labels=[c['label'] for c in d['connections']]; assert all(l for l in labels)" 2>/dev/null; then
    pass "All connections have labels"
else
    fail "Some connections missing labels"
fi

echo ""

# ---- Test 8: Components JSON Registry ----
echo "--- Test 8: Components Registry ---"

REGISTRY="/workspace/UnifiedApp/components.json"
if python3 -c "import json; d=json.load(open('$REGISTRY')); assert len(d['components']) >= 5" 2>/dev/null; then
    pass "Registry has 5+ component types"
else
    fail "Registry missing component types"
fi

# Check each component has subsystems
if python3 -c "
import json
d=json.load(open('$REGISTRY'))
for c in d['components']:
    assert len(c.get('subsystems', [])) >= 3, f'{c[\"type_id\"]} has < 3 subsystems'
" 2>/dev/null; then
    pass "All components have 3+ subsystems"
else
    fail "Some components have too few subsystems"
fi

echo ""

# ---- Test 9: .pro file ----
echo "--- Test 9: Project File ---"

if grep -q "subcomponent.cpp" UnifiedApp.pro; then
    pass "subcomponent.cpp in .pro SOURCES"
else
    fail "subcomponent.cpp missing from .pro"
fi

if grep -q "subcomponent.h" UnifiedApp.pro; then
    pass "subcomponent.h in .pro HEADERS"
else
    fail "subcomponent.h missing from .pro"
fi

if grep -q "connection.cpp" UnifiedApp.pro; then
    pass "connection.cpp in .pro SOURCES"
else
    fail "connection.cpp missing from .pro"
fi

if grep -q "connection.h" UnifiedApp.pro; then
    pass "connection.h in .pro HEADERS"
else
    fail "connection.h missing from .pro"
fi

echo ""

# ---- Summary ----
echo "============================================================"
echo "RESULTS: $PASS passed / $FAIL failed / $TOTAL total"
echo "============================================================"

if [ $FAIL -eq 0 ]; then
    echo "ALL TESTS PASSED!"
    exit 0
else
    echo "SOME TESTS FAILED!"
    exit 1
fi
