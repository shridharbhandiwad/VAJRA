# Subsystem Images - Quick Reference

## Directory Structure at a Glance

```
assets/subsystems/
│
├── 📄 README.md                    # Complete subsystem documentation
├── 📄 IMAGE_PLACEMENT_GUIDE.md     # Detailed placement instructions
├── 📄 QUICK_REFERENCE.md           # This file
│
├── 📁 antenna/
│   ├── 📄 README.md
│   ├── 🖼️  antenna_main.png         ← Place Image 5 here
│   └── 🖼️  antenna_thumbnail.png    ← 256x256 thumbnail of Image 5
│
├── 📁 communication_system/
│   ├── 📄 README.md
│   ├── 🖼️  comm_system_main.png     ← Place Image 1 here
│   └── 🖼️  comm_system_thumbnail.png ← 256x256 thumbnail of Image 1
│
├── 📁 liquid_cooling_unit/
│   ├── 📄 README.md
│   ├── 🖼️  cooling_unit_main.png    ← Place Image 4 here
│   └── 🖼️  cooling_unit_thumbnail.png ← 256x256 thumbnail of Image 4
│
├── 📁 power_system/
│   ├── 📄 README.md
│   ├── 🖼️  power_system_main.png    ← Place Image 3 here
│   └── 🖼️  power_system_thumbnail.png ← 256x256 thumbnail of Image 3
│
└── 📁 radar_computer/
    ├── 📄 README.md
    ├── 🖼️  radar_computer_main.png  ← Place Image 2 here
    └── 🖼️  radar_computer_thumbnail.png ← 256x256 thumbnail of Image 2
```

## Image Assignment Summary

| # | Visual Description | Subsystem | Directory | Files |
|---|-------------------|-----------|-----------|-------|
| 1️⃣ | Control room with multiple monitors | Communication System | `communication_system/` | `comm_system_main.png`<br>`comm_system_thumbnail.png` |
| 2️⃣ | Top-down technical component layout | Radar Computer | `radar_computer/` | `radar_computer_main.png`<br>`radar_computer_thumbnail.png` |
| 3️⃣ | Industrial metallic equipment unit | Power System | `power_system/` | `power_system_main.png`<br>`power_system_thumbnail.png` |
| 4️⃣ | Equipment with copper/bronze pipes | Liquid Cooling Unit | `liquid_cooling_unit/` | `cooling_unit_main.png`<br>`cooling_unit_thumbnail.png` |
| 5️⃣ | Satellite dish antenna | Antenna | `antenna/` | `antenna_main.png`<br>`antenna_thumbnail.png` |

## Component IDs in Application

| Subsystem Type | Default Component ID | Qt Type Enum |
|---------------|---------------------|--------------|
| Antenna | `antenna_1` | `ComponentType::Antenna` |
| Communication System | `comm_1` | `ComponentType::CommunicationSystem` |
| Liquid Cooling Unit | `cooling_1` | `ComponentType::LiquidCoolingUnit` |
| Power System | `power_1` | `ComponentType::PowerSystem` |
| Radar Computer | `computer_1` | `ComponentType::RadarComputer` |

## File Naming Pattern

```
[subsystem_name]_main.png         # Full-resolution image
[subsystem_name]_thumbnail.png    # 256x256 UI icon
```

Examples:
- ✅ `antenna_main.png`
- ✅ `cooling_unit_thumbnail.png`
- ❌ `Antenna_Main.PNG` (wrong case)
- ❌ `antenna-main.png` (use underscore, not hyphen)

## Quick Image Placement Steps

1. **Save Image 1** → `communication_system/comm_system_main.png`
2. **Save Image 2** → `radar_computer/radar_computer_main.png`
3. **Save Image 3** → `power_system/power_system_main.png`
4. **Save Image 4** → `liquid_cooling_unit/cooling_unit_main.png`
5. **Save Image 5** → `antenna/antenna_main.png`
6. **Create thumbnails** (256x256) for each image
7. **Commit**: `git add assets/ && git commit -m "Add subsystem images"`

## Generate Thumbnails (ImageMagick)

```bash
cd assets/subsystems

# Individual thumbnails
convert antenna/antenna_main.png -resize 256x256 -gravity center -extent 256x256 antenna/antenna_thumbnail.png
convert communication_system/comm_system_main.png -resize 256x256 -gravity center -extent 256x256 communication_system/comm_system_thumbnail.png
convert liquid_cooling_unit/cooling_unit_main.png -resize 256x256 -gravity center -extent 256x256 liquid_cooling_unit/cooling_unit_thumbnail.png
convert power_system/power_system_main.png -resize 256x256 -gravity center -extent 256x256 power_system/power_system_thumbnail.png
convert radar_computer/radar_computer_main.png -resize 256x256 -gravity center -extent 256x256 radar_computer/radar_computer_thumbnail.png
```

## Use in Qt Code

```cpp
// Load main image
QPixmap image("assets/subsystems/antenna/antenna_main.png");

// Load thumbnail
QIcon icon("assets/subsystems/antenna/antenna_thumbnail.png");

// Display in UI
label->setPixmap(image.scaled(800, 800, Qt::KeepAspectRatio));
listItem->setIcon(icon);
```

## Documentation Files

- 📖 **Complete Guide**: `README.md`
- 📋 **Placement Instructions**: `IMAGE_PLACEMENT_GUIDE.md`
- ⚡ **Quick Reference**: This file
- 🗂️ **Assets Index**: `../ASSETS_INDEX.md`
- 📝 **Implementation Summary**: `../../SUBSYSTEM_IMAGES_LAYOUT.md`

## Checklist

- [x] Directory structure created
- [x] Documentation written
- [x] Git tracking enabled
- [ ] Images placed
- [ ] Thumbnails generated
- [ ] Application code updated
- [ ] Integration tested

---

**For detailed instructions, see**: `IMAGE_PLACEMENT_GUIDE.md`
