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

# Install TTS dependencies for voice health alerts
echo "Checking voice alert dependencies..."
NEED_INSTALL=""

if ! command -v espeak-ng &> /dev/null && ! command -v espeak &> /dev/null; then
    echo "  espeak-ng: NOT FOUND"
    NEED_INSTALL="espeak-ng"
else
    echo "  TTS engine: $(command -v espeak-ng 2>/dev/null || command -v espeak 2>/dev/null)"
fi

if ! command -v aplay &> /dev/null; then
    echo "  aplay: NOT FOUND"
    NEED_INSTALL="$NEED_INSTALL alsa-utils"
else
    echo "  Audio player: $(command -v aplay)"
fi

if [ -n "$NEED_INSTALL" ]; then
    echo ""
    echo "Installing missing voice dependencies: $NEED_INSTALL"
    if command -v apt-get &> /dev/null; then
        sudo apt-get install -y $NEED_INSTALL 2>/dev/null
        if [ $? -eq 0 ]; then
            echo "Voice dependencies installed successfully!"
        else
            echo "WARNING: Could not auto-install voice dependencies."
            echo "Install manually: sudo apt-get install espeak-ng alsa-utils"
        fi
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y $NEED_INSTALL 2>/dev/null || echo "Install manually: sudo dnf install espeak-ng alsa-utils"
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --noconfirm $NEED_INSTALL 2>/dev/null || echo "Install manually: sudo pacman -S espeak-ng alsa-utils"
    else
        echo "WARNING: Could not detect package manager."
        echo "Install manually: espeak-ng and alsa-utils"
    fi
else
    echo "  All voice dependencies present!"
fi

# Verify TTS after installation
echo ""
if command -v espeak-ng &> /dev/null; then
    echo "Voice Alerts: espeak-ng ready ($(espeak-ng --version 2>/dev/null | head -1))"
elif command -v espeak &> /dev/null; then
    echo "Voice Alerts: espeak ready"
else
    echo "Voice Alerts: No TTS engine available - alerts will be logged only"
    echo "  Install: sudo apt-get install espeak-ng alsa-utils"
fi
echo

# Build Unified Application
echo "Building Unified Application..."
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
echo "======================================"
echo "Build completed successfully!"
echo "======================================"
echo
echo "Executable:"
echo "  - UnifiedApp/UnifiedApp          (Single app with role-based access)"
echo
echo "To run:"
echo "  ./UnifiedApp/UnifiedApp"
echo "  Login with: Designer/designer (design mode) or User/user (runtime mode)"
echo
echo "Voice Health Alerts:"
echo "  Runtime mode announces 'System Status Critical/Degraded <health>%'"
echo "  when subsystem health drops."
echo "  - Toggle via VOICE ALERTS button in toolbar"
echo "  - Click TEST VOICE to verify audio output"
echo "  - Uses multi-strategy TTS: pipeline (espeak|aplay), WAV file, or direct"
echo "  - Requires: espeak-ng + alsa-utils (auto-installed during build)"
echo "  - Manual install: sudo apt-get install espeak-ng alsa-utils"
echo
