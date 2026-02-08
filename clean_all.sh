#!/bin/bash
# Clean script for all Qt applications

echo "Cleaning build artifacts..."

# Clean Designer Application
if [ -d "DesignerApp" ]; then
    cd DesignerApp
    make clean 2>/dev/null
    rm -f Makefile DesignerApp .qmake.stash
    rm -rf moc_* ui_* qrc_* *.o
    cd ..
    echo "✓ Designer Application cleaned"
fi

# Clean Runtime Application
if [ -d "RuntimeApp" ]; then
    cd RuntimeApp
    make clean 2>/dev/null
    rm -f Makefile RuntimeApp .qmake.stash
    rm -rf moc_* ui_* qrc_* *.o
    cd ..
    echo "✓ Runtime Application cleaned"
fi

echo "Clean completed!"
