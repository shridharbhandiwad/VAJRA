# Assets Index

This directory contains all visual and media assets for the Radar System Monitoring Application.

## Directory Structure

```
assets/
├── ASSETS_INDEX.md (this file)
└── subsystems/
    ├── README.md                          # Comprehensive subsystems documentation
    ├── IMAGE_PLACEMENT_GUIDE.md           # Detailed image placement instructions
    ├── antenna/
    ├── communication_system/
    ├── liquid_cooling_unit/
    ├── power_system/
    └── radar_computer/
```

## Asset Categories

### Subsystem Images (`subsystems/`)
Visual representations of the five radar subsystems:
1. **Antenna** - Satellite dish and signal transmission equipment
2. **Communication System** - Control room and network infrastructure
3. **Liquid Cooling Unit** - Thermal management equipment
4. **Power System** - Electrical power distribution equipment
5. **Radar Computer** - Processing and control systems

**Status**: Directory structure created, awaiting image placement
**Reference**: See `subsystems/IMAGE_PLACEMENT_GUIDE.md` for detailed placement instructions

## File Organization Principles

1. **Component-Based Layout**: Assets organized by component/subsystem type
2. **Consistent Naming**: Standardized file naming conventions
3. **Documentation**: Each directory includes README with usage details
4. **Variants**: Main images and thumbnails separated clearly
5. **Scalability**: Easy to extend with new asset types or subsystems

## Future Asset Categories

Potential additions to the assets directory:

- `icons/` - Application icons and UI elements
- `logos/` - Company/product logos
- `diagrams/` - System architecture and flow diagrams
- `animations/` - Animated visual elements
- `sounds/` - Audio alerts and notifications
- `fonts/` - Custom typography
- `themes/` - Color schemes and style assets
- `models/` - 3D models for advanced visualization

## Usage in Applications

Assets in this directory are referenced by:
- **UnifiedApp** - Component list, canvas visualization, real-time monitoring displays, detailed views

### Asset Loading Example

```cpp
// Load subsystem image
QString assetPath = "assets/subsystems/antenna/antenna_main.png";
QPixmap image(assetPath);

// Display in UI
imageLabel->setPixmap(image.scaled(800, 800, Qt::KeepAspectRatio));
```

## Version Control

- All asset directories are tracked in git
- Binary files (images) should be optimized before committing
- Large files (>10MB) may require Git LFS
- Use `.gitignore` for temporary or generated assets

## Contributing Assets

When adding new assets:

1. Place in appropriate category directory
2. Follow existing naming conventions
3. Update relevant README files
4. Add documentation about usage
5. Optimize file sizes
6. Commit with descriptive message

## Asset Guidelines

### Images
- **Format**: PNG (with transparency) or JPG (photographic)
- **Resolution**: High enough for intended use, but optimized
- **Color Space**: RGB (not CMYK)
- **Naming**: lowercase_with_underscores.png

### Icons
- **Size**: Multiple sizes (16x16, 32x32, 64x64, 128x128, 256x256)
- **Format**: PNG with transparency or SVG
- **Style**: Consistent with application design language

### Documentation Images
- **Format**: PNG or SVG
- **Purpose**: Clear visual communication
- **Annotations**: Use when helpful for clarity

---

**Last Updated**: February 8, 2026
**Maintained By**: Radar System Development Team
