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

# Build Designer Application
echo "Building Designer Application..."
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
echo "Building Runtime Application..."
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
echo "  - DesignerApp/DesignerApp"
echo "  - RuntimeApp/RuntimeApp"
echo
echo "To run:"
echo "  ./DesignerApp/DesignerApp"
echo "  ./RuntimeApp/RuntimeApp"
echo
