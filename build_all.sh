#!/bin/bash
# Build script for all Qt applications

echo "======================================"
echo "Building Qt Applications"
echo "======================================"
echo

# Check if qmake is available
if ! command -v qmake &> /dev/null; then
    echo "Error: qmake not found. Please install Qt development tools."
    echo "On Ubuntu/Debian: sudo apt-get install qt5-qmake qtbase5-dev qtbase5-dev-tools"
    echo "On Fedora: sudo dnf install qt5-qtbase-devel"
    exit 1
fi

# Check for TTS engine (optional - for voice health alerts)
if command -v espeak-ng &> /dev/null; then
    echo "Voice Alerts: espeak-ng detected"
elif command -v espeak &> /dev/null; then
    echo "Voice Alerts: espeak detected"
else
    echo "Note: No TTS engine found. Voice health alerts will be logged only."
    echo "Install espeak-ng for voice alerts: sudo apt-get install espeak-ng"
fi
echo

# Build Unified Application (Recommended)
echo "Building Unified Application (Recommended)..."
cd UnifiedApp || exit 1
qmake UnifiedApp.pro
make
if [ $? -eq 0 ]; then
    echo "✓ Unified Application built successfully"
else
    echo "✗ Unified Application build failed"
    exit 1
fi
cd ..

echo

# Build Designer Application
echo "Building Designer Application (Legacy)..."
cd DesignerApp || exit 1
qmake DesignerApp.pro
make
if [ $? -eq 0 ]; then
    echo "✓ Designer Application built successfully"
else
    echo "✗ Designer Application build failed"
    exit 1
fi
cd ..

echo

# Build Runtime Application
echo "Building Runtime Application (Legacy)..."
cd RuntimeApp || exit 1
qmake RuntimeApp.pro
make
if [ $? -eq 0 ]; then
    echo "✓ Runtime Application built successfully"
else
    echo "✗ Runtime Application build failed"
    exit 1
fi
cd ..

echo
echo "======================================"
echo "Build completed successfully!"
echo "======================================"
echo
echo "Executables:"
echo "  - UnifiedApp/UnifiedApp          (RECOMMENDED - Single app with role-based access)"
echo "  - DesignerApp/DesignerApp         (Legacy - Separate designer)"
echo "  - RuntimeApp/RuntimeApp           (Legacy - Separate runtime monitor)"
echo
echo "To run Unified Application (Recommended):"
echo "  ./UnifiedApp/UnifiedApp"
echo "  Login with: Designer/designer (design mode) or User/user (runtime mode)"
echo
echo "To run separate applications:"
echo "  ./DesignerApp/DesignerApp"
echo "  ./RuntimeApp/RuntimeApp"
echo
echo "Voice Health Alerts:"
echo "  Runtime mode announces 'System Status Critical/Degraded <health>%'"
echo "  when subsystem health drops. Toggle via VOICE ALERTS button in toolbar."
echo "  Requires: espeak-ng (sudo apt-get install espeak-ng)"
echo
