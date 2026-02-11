# Advanced Analytics Dashboard - User Guide

## Overview

A comprehensive, visually rich analytics dashboard has been added to the Unified Application. The dashboard provides real-time monitoring and analysis of component health, performance metrics, and system telemetry data.

## Features

### 🎨 Rich UI & Aesthetics

- **Modern Design**: Clean, professional interface with card-based KPI displays
- **Responsive Layout**: Grid-based layout with scrollable content for all screen sizes
- **Theme Support**: Automatic dark/light theme switching that syncs with the main application
- **Smooth Animations**: Chart animations for better visual appeal
- **Color-Coded Data**: Health-based color coding (green, blue, yellow, orange, red)

### 📊 10 Different Chart Types

1. **Health Trend Chart** (Multi-line Spline Chart)
   - Real-time health monitoring of up to 6 components
   - Shows last 24 hours of health data
   - Smooth spline curves for better readability
   - Color-coded component lines

2. **Component Distribution Chart** (Donut Pie Chart)
   - Visual breakdown of component types
   - Interactive slices with labels
   - Percentage distribution display
   - Donut style (35% hole size) for modern look

3. **Subsystem Performance Chart** (Bar Chart)
   - Average health of each subsystem type
   - Horizontal comparison across subsystems
   - Color-coded bars (blue theme)

4. **Health Distribution Over Time** (Stacked Area Chart)
   - Shows component count in different health zones
   - Critical (<60%), Warning (60-80%), Good (>80%)
   - Time-series visualization
   - Semi-transparent filled areas

5. **Message Frequency Analysis** (Scatter Plot)
   - Message events over last 6 hours
   - Component-wise message distribution
   - Event clustering visualization
   - 8-pixel markers for clarity

6. **Telemetry Data Streams** (Multi-line Chart)
   - Temperature, Power, and Signal Strength metrics
   - Last 100 minutes of telemetry data
   - Three distinct color-coded lines
   - Real-time streaming data

7. **Alert History** (Grouped Bar Chart)
   - Critical vs Warning alerts
   - Hourly breakdown for last 12 hours
   - Stacked comparison view
   - Color-coded by severity (red/yellow)

8. **Component Performance Comparison** (Horizontal Bar Chart)
   - Current health of up to 8 components
   - Side-by-side comparison
   - Easy visual ranking
   - Green color scheme

9. **System Efficiency Metrics** (Area Chart with Gradient)
   - Overall system efficiency over time
   - Smooth gradient fill (purple theme)
   - Target range visualization
   - Last 100 minutes of data

10. **Component Uptime Analysis** (Stacked Bar Chart)
    - Uptime vs Downtime percentage
    - Per-component breakdown
    - Green (uptime) and Red (downtime) coding
    - Percentage-based visualization

### 📈 KPI Dashboard Cards

Six prominent KPI cards display key metrics:

1. **Total Components** (Blue)
   - Number of monitored systems
   
2. **Active Components** (Green)
   - Currently online components
   
3. **Average Health** (Yellow)
   - System-wide health percentage
   
4. **Total Alerts** (Red)
   - Alert count in last 24 hours
   
5. **System Efficiency** (Purple)
   - Performance index percentage
   
6. **Uptime** (Teal)
   - Availability percentage

### ⚙️ Interactive Controls

- **Time Range Selector**: Last Hour, 6 Hours, 24 Hours, Week, Month
- **Component Filter**: Filter dashboard by specific component
- **Refresh Button**: Manual dashboard refresh
- **Export Data Button**: Export analytics data (placeholder for future implementation)
- **Auto-Refresh**: Automatic updates every 5 seconds

## Access

### Opening the Dashboard

1. Launch the Unified Application
2. Log in with any user role (Designer or User)
3. Look for the **"📊 VIEW DASHBOARD"** button in the top toolbar
4. Click the button to open the Analytics Dashboard in a new window

The dashboard button is visible to all user roles and appears between the theme toggle and log off buttons.

## Sample Data

The dashboard includes comprehensive sample data generation to demonstrate functionality:

- **8-12 sample components** of various types (Radar, Antenna, PowerSystem, Communication, CoolingUnit, Computer)
- **5 subsystems per component** (Transmitter, Receiver, Processor, Monitor, Controller)
- **24 hours of health history** (288 data points, sampled every 5 minutes)
- **Realistic health variations** with occasional drops to simulate issues
- **Message timestamp tracking** for frequency analysis
- **Alert generation** based on health thresholds

## Technical Details

### Architecture

```
AnalyticsDashboard (QMainWindow)
├── Header Section
│   ├── Title and Subtitle
│   └── Control Panel (Time Range, Filter, Actions)
├── KPI Section
│   └── 6 KPI Cards (Metrics Display)
└── Charts Section (5 Rows)
    ├── Row 1: Health Trend (2/3) + Component Distribution (1/3)
    ├── Row 2: Subsystem Performance (1/2) + Health Area (1/2)
    ├── Row 3: Message Frequency (1/2) + Telemetry (1/2)
    ├── Row 4: Alert History (1/2) + Component Comparison (1/2)
    └── Row 5: System Efficiency (1/2) + Uptime (1/2)
```

### Data Structures

```cpp
struct ComponentHealthData {
    QString componentId;
    QString type;
    QVector<QPair<qint64, qreal>> healthHistory;
    QMap<QString, QVector<qreal>> subsystemHealth;
    QVector<qint64> messageTimestamps;
    qreal currentHealth;
    QString currentStatus;
    int totalMessages;
    int alertCount;
    qint64 lastUpdateTime;
};
```

### Qt Charts Dependencies

The dashboard uses Qt Charts module with the following chart types:
- QLineSeries / QSplineSeries
- QAreaSeries
- QBarSeries / QBarSet
- QPieSeries / QPieSlice
- QScatterSeries
- QValueAxis / QBarCategoryAxis

## Building & Running

### Prerequisites

```bash
# Qt 5.x or Qt 6.x with Charts module
sudo apt-get install qt5-default qt5-qmake qtbase5-dev qtcharts5-dev
# OR for Qt 6
sudo apt-get install qt6-base-dev qt6-charts-dev
```

### Build

```bash
cd UnifiedApp
qmake UnifiedApp.pro
make
./UnifiedApp
```

### Dependencies

- Qt Core
- Qt GUI
- Qt Widgets
- Qt Network
- **Qt Charts** (NEW)

## Integration with Existing System

The dashboard is designed to integrate seamlessly with the existing component monitoring system:

1. **Auto-population**: When opened, the dashboard automatically populates with components from the canvas
2. **Future enhancements**: Can connect to MessageServer signals for real-time data updates
3. **Theme synchronization**: Automatically switches theme when main window theme changes
4. **Independent window**: Opens in separate window, can be positioned on second monitor

## Future Enhancement Possibilities

1. **Real-time data integration**: Connect to actual component health data from MessageServer
2. **Export functionality**: CSV/PDF export of analytics data
3. **Custom date ranges**: User-defined time range selection
4. **Drill-down views**: Click chart elements for detailed component view
5. **Alert configuration**: Set custom alert thresholds
6. **Historical data persistence**: Save and load historical analytics data
7. **Comparison modes**: Compare time periods or component groups
8. **Predictive analytics**: Trend forecasting and anomaly detection

## Customization

### Adding New Charts

To add a new chart type:

1. Add chart view member variable in `analyticsdashboard.h`
2. Create chart creation method (e.g., `createNewChart()`)
3. Add chart to appropriate row in `setupUI()`
4. Update chart data in `updateCharts()`
5. Apply theme in `applyChartTheme()`

### Modifying Sample Data

Adjust sample data generation in `generateSampleData()`:

```cpp
// Change number of components
int numComponents = 8 + rng->bounded(5); // 8-12 components

// Change history duration
int numPoints = 288; // 24 hours * 12 points per hour

// Change health variation
qreal healthVariation = (rng->bounded(1000) / 1000.0 - 0.5) * 10;
```

## Color Scheme

### Health Status Colors
- **Excellent (≥90%)**: Green #2ecc71
- **Good (≥75%)**: Blue #3498db
- **Fair (≥60%)**: Yellow #f1c40f
- **Poor (≥40%)**: Orange #e67e22
- **Critical (<40%)**: Red #e74c3c

### KPI Card Colors
- Blue: #3498db
- Green: #2ecc71
- Yellow: #f1c40f
- Red: #e74c3c
- Purple: #9b59b6
- Teal: #1abc9a

### Theme Colors

**Dark Theme:**
- Background: #1e1e1e
- Text: #dcdcdc
- Grid: #3c3c3c
- Chart Background: #2d2d2d

**Light Theme:**
- Background: #f5f5f5
- Text: #282828
- Grid: #c8c8c8
- Chart Background: #ffffff

## Troubleshooting

### Dashboard doesn't open
- Check if Qt Charts module is properly installed
- Verify the application was built with Qt Charts support
- Check console for error messages

### Charts not displaying data
- Ensure sample data generation completed successfully
- Check if `updateCharts()` is being called
- Verify chart series are properly attached to axes

### Theme not applying correctly
- Ensure ThemeManager is properly initialized
- Check if theme signal connections are established
- Verify `applyChartTheme()` is called for all charts

## Performance Considerations

- Charts are optimized to show every 10th-20th data point for better performance
- Maximum history size is limited (1000 points per component)
- Auto-refresh runs every 5 seconds (configurable)
- Maximum 6 series displayed in health trend chart for readability
- Scroll area ensures smooth navigation with many charts

## Conclusion

The Analytics Dashboard provides a comprehensive, visually appealing way to monitor system health and performance. With 10 different chart types, 6 KPI cards, and rich sample data, it offers deep insights into component behavior and system trends.

**Enjoy your new analytics capabilities!** 📊✨
