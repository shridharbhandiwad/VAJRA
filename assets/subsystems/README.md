# Radar Subsystem Images - Components Layout

This directory contains visual representations for the five radar subsystems used in the monitoring application.

## Directory Structure

```
assets/subsystems/
├── README.md                          # This file
├── antenna/
│   ├── antenna_main.png              # Primary antenna visualization (satellite dish)
│   └── antenna_thumbnail.png         # Thumbnail for UI components
├── power_system/
│   ├── power_system_main.png         # Power system equipment visualization
│   └── power_system_thumbnail.png    # Thumbnail for UI components
├── liquid_cooling_unit/
│   ├── cooling_unit_main.png         # Cooling unit with pipes and radiators
│   └── cooling_unit_thumbnail.png    # Thumbnail for UI components
├── communication_system/
│   ├── comm_system_main.png          # Communication/control room visualization
│   └── comm_system_thumbnail.png     # Thumbnail for UI components
└── radar_computer/
    ├── radar_computer_main.png       # Radar computer internal layout
    └── radar_computer_thumbnail.png  # Thumbnail for UI components
```

## Subsystem Image Mapping

### 1. Antenna (`antenna/`)
- **Type**: Antenna
- **Component ID**: `antenna_1` (default)
- **Description**: Satellite dish antenna system with support structure
- **Visual Features**: 
  - Parabolic dish reflector
  - Feed horn assembly
  - Mounting platform
  - Tracking mechanisms
- **Use Cases**: Signal transmission and reception visualization

### 2. Power System (`power_system/`)
- **Type**: PowerSystem
- **Component ID**: `power_1` (default)
- **Description**: Industrial power distribution and management unit
- **Visual Features**:
  - Power distribution panels
  - Voltage regulation equipment
  - Control interfaces
  - Safety systems
- **Use Cases**: Electrical power subsystem monitoring

### 3. Liquid Cooling Unit (`liquid_cooling_unit/`)
- **Type**: LiquidCoolingUnit
- **Component ID**: `cooling_1` (default)
- **Description**: Industrial cooling system with piping and heat exchange
- **Visual Features**:
  - Cooling pipes and conduits
  - Heat exchangers
  - Circulation pumps
  - Radiator systems
- **Use Cases**: Thermal management visualization

### 4. Communication System (`communication_system/`)
- **Type**: CommunicationSystem
- **Component ID**: `comm_1` (default)
- **Description**: Control room and communication infrastructure
- **Visual Features**:
  - Multi-monitor displays
  - Control stations
  - Network equipment
  - Data visualization screens
- **Use Cases**: Command and control center representation

### 5. Radar Computer (`radar_computer/`)
- **Type**: RadarComputer
- **Component ID**: `computer_1` (default)
- **Description**: Internal component layout of radar processing computer
- **Visual Features**:
  - Circuit boards and processors
  - Data processing modules
  - Control interfaces
  - Internal component arrangement
- **Use Cases**: Processing unit technical documentation

## Image Specifications

### Main Images
- **Format**: PNG (preferred) or JPG
- **Recommended Size**: 1024x1024 pixels
- **Aspect Ratio**: 1:1 (square) or 4:3
- **Color Space**: RGB
- **Use**: Detailed views, documentation, training materials

### Thumbnail Images
- **Format**: PNG with transparency
- **Recommended Size**: 256x256 pixels
- **Aspect Ratio**: 1:1 (square)
- **Background**: Transparent or solid color
- **Use**: UI component lists, quick references, icons

## Integration with Applications

### Designer Application
The images can be used to:
- Enhance component list with visual previews
- Provide reference documentation
- Create more realistic canvas representations
- Support user training and onboarding

### Runtime Application
The images can be used to:
- Display detailed subsystem views on hover/click
- Provide contextual help and documentation
- Support troubleshooting with visual references
- Create maintenance documentation

### Usage Example in Qt

```cpp
// Load subsystem image
QPixmap antennaImage("assets/subsystems/antenna/antenna_main.png");

// Load thumbnail for component list
QIcon antennaIcon("assets/subsystems/antenna/antenna_thumbnail.png");

// Display in UI
imageLabel->setPixmap(antennaImage.scaled(800, 800, Qt::KeepAspectRatio));
```

## Components Layout Philosophy

The components layout approach organizes subsystem assets by:

1. **Subsystem Type** - Each subsystem has its own directory
2. **Image Variant** - Main images and thumbnails are clearly separated
3. **Naming Convention** - Consistent, descriptive file names
4. **Modularity** - Easy to add new subsystems or image variants
5. **Documentation** - Each subsystem is well-documented with metadata

## Adding New Subsystem Images

To add images for a new subsystem:

1. Create a new subdirectory: `assets/subsystems/[subsystem_name]/`
2. Add main image: `[subsystem_name]_main.png`
3. Add thumbnail: `[subsystem_name]_thumbnail.png`
4. Update this README with subsystem details
5. Update application code to reference new images

## File Naming Conventions

- Use lowercase with underscores: `liquid_cooling_unit_main.png`
- Main images: `[subsystem]_main.[ext]`
- Thumbnails: `[subsystem]_thumbnail.[ext]`
- Variations: `[subsystem]_[variant]_[type].[ext]`
  - Example: `antenna_closeup_main.png`
  - Example: `power_system_schematic_thumbnail.png`

## Version Control

- All images should be committed to the repository
- Large images (>5MB) should be optimized before committing
- Use PNG for images requiring transparency
- Use JPG for photographic content without transparency

## Future Enhancements

Potential additions to the components layout:

- [ ] 3D model files (.obj, .stl) for each subsystem
- [ ] Schematic diagrams (SVG format)
- [ ] Animation sequences for subsystem operations
- [ ] AR/VR compatible models
- [ ] Interactive component breakdowns
- [ ] Maintenance procedure visuals
- [ ] Historical comparison images

## License and Attribution

These images are part of the Radar System Monitoring Application. Ensure proper attribution if using stock images or third-party assets.

---

**Last Updated**: February 8, 2026
**Maintained By**: Radar System Development Team
