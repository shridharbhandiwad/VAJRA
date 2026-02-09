#!/bin/bash
# Clean script for all Qt applications

echo "Cleaning build artifacts..."

# Clean Unified Application
if [ -d "UnifiedApp" ]; then
    cd UnifiedApp
    make clean 2>/dev/null
    rm -f Makefile UnifiedApp .qmake.stash
    rm -rf moc_* ui_* qrc_* *.o
    cd ..
    echo "✓ Unified Application cleaned"
fi

echo "Clean completed!"
