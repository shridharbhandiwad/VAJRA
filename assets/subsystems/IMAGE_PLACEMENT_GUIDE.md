# Image Placement Guide - Subsystem Components Layout

This guide maps the five uploaded subsystem images to their proper locations in the components layout structure.

## Image to Subsystem Mapping

### Image 1: Control Room / Command Center
**Target Directory**: `communication_system/`

**Files to create**:
- `communication_system/comm_system_main.png` ← Place Image 1 here
- `communication_system/comm_system_thumbnail.png` ← Create 256x256 thumbnail from Image 1

**Description**: Multi-monitor control room with cyan/blue ambient lighting, operator workstations, and network equipment displays.

**Component Details**:
- Subsystem: Communication System
- Type: `CommunicationSystem`
- Default ID: `comm_1`
- Function: Data transmission and network connectivity

---

### Image 2: Internal Technical Layout (Top-Down View)
**Target Directory**: `radar_computer/`

**Files to create**:
- `radar_computer/radar_computer_main.png` ← Place Image 2 here
- `radar_computer/radar_computer_thumbnail.png` ← Create 256x256 thumbnail from Image 2

**Description**: Top-down technical view showing organized internal components, circuit boards, control interfaces, and modular assemblies.

**Component Details**:
- Subsystem: Radar Computer
- Type: `RadarComputer`
- Default ID: `computer_1`
- Function: Radar data processing and system control

---

### Image 3: Industrial Equipment Unit (Metallic/Silver)
**Target Directory**: `power_system/`

**Files to create**:
- `power_system/power_system_main.png` ← Place Image 3 here
- `power_system/power_system_thumbnail.png` ← Create 256x256 thumbnail from Image 3

**Description**: Industrial equipment with metallic frame, control panels, pressure vessels, and precision engineering components.

**Component Details**:
- Subsystem: Power System
- Type: `PowerSystem`
- Default ID: `power_1`
- Function: Electrical power distribution and management

---

### Image 4: Industrial Equipment with Pipes (Copper/Bronze Conduits)
**Target Directory**: `liquid_cooling_unit/`

**Files to create**:
- `liquid_cooling_unit/cooling_unit_main.png` ← Place Image 4 here
- `liquid_cooling_unit/cooling_unit_thumbnail.png` ← Create 256x256 thumbnail from Image 4

**Description**: Industrial cooling system featuring extensive copper/bronze colored piping, heat exchangers, pumps, and radiator assemblies.

**Component Details**:
- Subsystem: Liquid Cooling Unit
- Type: `LiquidCoolingUnit`
- Default ID: `cooling_1`
- Function: Thermal management and temperature regulation

---

### Image 5: Satellite Dish Antenna
**Target Directory**: `antenna/`

**Files to create**:
- `antenna/antenna_main.png` ← Place Image 5 here
- `antenna/antenna_thumbnail.png` ← Create 256x256 thumbnail from Image 5

**Description**: Large parabolic satellite dish with reflector, feed horn, mounting platform, and illuminated circular base.

**Component Details**:
- Subsystem: Antenna
- Type: `Antenna`
- Default ID: `antenna_1`
- Function: Radar signal transmission and reception

---

## Directory Structure After Placement

```
assets/subsystems/
├── README.md
├── IMAGE_PLACEMENT_GUIDE.md (this file)
│
├── antenna/
│   ├── README.md
│   ├── antenna_main.png                    ← Image 5
│   └── antenna_thumbnail.png               ← Image 5 (thumbnail)
│
├── communication_system/
│   ├── README.md
│   ├── comm_system_main.png                ← Image 1
│   └── comm_system_thumbnail.png           ← Image 1 (thumbnail)
│
├── liquid_cooling_unit/
│   ├── README.md
│   ├── cooling_unit_main.png               ← Image 4
│   └── cooling_unit_thumbnail.png          ← Image 4 (thumbnail)
│
├── power_system/
│   ├── README.md
│   ├── power_system_main.png               ← Image 3
│   └── power_system_thumbnail.png          ← Image 3 (thumbnail)
│
└── radar_computer/
    ├── README.md
    ├── radar_computer_main.png             ← Image 2
    └── radar_computer_thumbnail.png        ← Image 2 (thumbnail)
```

## Image Processing Instructions

### For Main Images
1. Save the original uploaded image
2. Rename according to the mapping above
3. Recommended format: PNG (lossless)
4. Recommended size: Keep original resolution (appears to be ~1024x1024)
5. Optimize file size if needed (use tools like pngquant, optipng)

### For Thumbnail Images
1. Create 256x256 pixel version of main image
2. Maintain aspect ratio (crop if needed to achieve square format)
3. Use PNG format with transparency if applicable
4. Optimize for small file size while maintaining clarity
5. Consider adding subtle border or background if needed for UI

## Command-Line Thumbnail Generation

If you have ImageMagick installed, you can create thumbnails with:

```bash
# Example for antenna
convert antenna/antenna_main.png -resize 256x256 -gravity center -extent 256x256 antenna/antenna_thumbnail.png

# Batch process all subsystems
for dir in antenna communication_system liquid_cooling_unit power_system radar_computer; do
  cd assets/subsystems/$dir
  for img in *_main.png; do
    convert $img -resize 256x256 -gravity center -extent 256x256 ${img/_main/_thumbnail}
  done
  cd -
done
```

## Validation Checklist

After placing all images, verify:

- [ ] All 5 main images are in their correct directories
- [ ] All 5 thumbnail images have been created
- [ ] File naming follows convention: `[subsystem]_main.png` and `[subsystem]_thumbnail.png`
- [ ] Images are in PNG format
- [ ] File sizes are reasonable (<5MB for main, <500KB for thumbnails)
- [ ] README files exist in each subdirectory
- [ ] Git tracking is enabled for all directories

## Git Commands for Committing

```bash
# Stage all new images
git add assets/subsystems/

# Commit with descriptive message
git commit -m "Add subsystem component images with organized layout structure

- Added 5 main subsystem images (antenna, power, cooling, comm, computer)
- Created 256x256 thumbnails for UI integration
- Organized in components layout with per-subsystem directories
- Added comprehensive documentation and placement guides"

# Push to feature branch
git push -u origin cursor/subsystem-images-layout-3661
```

## Integration with Application Code

Once images are placed, you can integrate them into the Qt applications:

### Example: Load Subsystem Image in Component Class

```cpp
// In component.cpp
QPixmap Component::loadSubsystemImage() const {
    QString imagePath;
    
    switch(type) {
        case ComponentType::Antenna:
            imagePath = "assets/subsystems/antenna/antenna_main.png";
            break;
        case ComponentType::PowerSystem:
            imagePath = "assets/subsystems/power_system/power_system_main.png";
            break;
        case ComponentType::LiquidCoolingUnit:
            imagePath = "assets/subsystems/liquid_cooling_unit/cooling_unit_main.png";
            break;
        case ComponentType::CommunicationSystem:
            imagePath = "assets/subsystems/communication_system/comm_system_main.png";
            break;
        case ComponentType::RadarComputer:
            imagePath = "assets/subsystems/radar_computer/radar_computer_main.png";
            break;
    }
    
    return QPixmap(imagePath);
}
```

### Example: Load Thumbnail for Component List

```cpp
// In componentlist.cpp
QIcon ComponentList::getSubsystemIcon(ComponentType type) const {
    QString iconPath;
    
    switch(type) {
        case ComponentType::Antenna:
            iconPath = "assets/subsystems/antenna/antenna_thumbnail.png";
            break;
        case ComponentType::PowerSystem:
            iconPath = "assets/subsystems/power_system/power_system_thumbnail.png";
            break;
        // ... etc
    }
    
    return QIcon(iconPath);
}
```

---

**Created**: February 8, 2026
**Purpose**: Components Layout Organization for Subsystem Images
**Related Issue**: #3661
