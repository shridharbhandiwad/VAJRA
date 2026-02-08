# Unified Radar System Application

A single, unified application that combines both the Designer and Runtime monitoring capabilities of the Radar System. Access control is managed through role-based login credentials.

## Overview

This application replaces the separate DesignerApp and RuntimeApp with a single, unified solution. Based on login credentials, users are granted appropriate access:

- **Designer Mode**: Full design capabilities including creating, editing, and saving radar system layouts
- **Runtime Mode**: Real-time monitoring of radar subsystem health status

## Login Credentials

### Designer Access
- **Username**: `Designer`
- **Password**: `designer`
- **Capabilities**: 
  - Design radar system layouts
  - Drag and drop subsystem components
  - Save and load designs
  - Clear canvas
  - View analytics

### User Access (Runtime Monitoring)
- **Username**: `User`
- **Password**: `user`
- **Capabilities**:
  - Load existing radar system layouts
  - Monitor subsystem health in real-time
  - Receive updates from external systems via TCP (port 12345)
  - View analytics and status information

## Features

### Common Features
- Modern, intuitive user interface
- Analytics panel showing component statistics
- Load existing radar system designs
- Real-time visualization

### Designer-Only Features
- Component palette with drag-and-drop functionality
- Save designs to `.design` files
- Create new system layouts
- Edit existing layouts
- Clear canvas

### Runtime-Only Features
- TCP message server (port 12345) for receiving health updates
- Real-time component status visualization
- Client connection monitoring
- Health color indicators based on external system data

## Building the Application

### Prerequisites
- Qt5 development libraries
- C++11 compatible compiler
- qmake build tool

### Installation on Ubuntu/Debian
```bash
sudo apt-get install qt5-qmake qtbase5-dev qtbase5-dev-tools
```

### Building
```bash
cd UnifiedApp
qmake UnifiedApp.pro
make
```

## Running the Application

```bash
./UnifiedApp
```

Upon launch, a login dialog will appear. Enter your credentials to access the appropriate mode.

## Architecture

The unified application uses role-based access control to determine the user interface and functionality:

1. **Login Dialog** (`logindialog.h/cpp`)
   - Authenticates user credentials
   - Determines user role (Designer or User)
   - Passes role information to main window

2. **Main Window** (`mainwindow.h/cpp`)
   - Adapts UI based on user role
   - Loads appropriate components and features
   - Manages all application functionality

3. **Canvas** (`canvas.h/cpp`)
   - Unified canvas supporting both design and runtime modes
   - Handles drag-and-drop (Designer mode)
   - Updates component visuals from TCP messages (Runtime mode)

4. **Components** (`component.h/cpp`)
   - Radar subsystem visual representations
   - Support for different types: Antenna, Power System, Liquid Cooling Unit, Communication System, Radar Computer

5. **Message Server** (`messageserver.h/cpp`)
   - TCP server for Runtime mode
   - Receives health status updates from external systems
   - Updates component visuals in real-time

6. **Analytics** (`analytics.h/cpp`)
   - Real-time statistics and component tracking
   - Message history (Runtime mode)
   - Component count and type information

## Component Types

The application supports the following radar subsystem types:
- **Antenna**: Radar signal transmission/reception
- **Power System**: Power distribution and management
- **Liquid Cooling Unit**: Thermal management
- **Communication System**: Data communication interfaces
- **Radar Computer**: Processing and control

## File Format

Designs are saved in JSON format (`.design` files) containing:
- Component IDs and types
- Position coordinates
- Visual properties (color, size)

## Integration with External Systems

In Runtime mode, the application listens on TCP port 12345 for health status messages. External systems can send JSON-formatted messages:

```json
{
    "component_id": "component_1",
    "color": "#00ff00",
    "size": 50.0
}
```

See `ExternalSystems/` directory for example external system simulators.

## Security Notes

- Current implementation uses hardcoded credentials for demonstration purposes
- For production use, implement proper authentication mechanisms
- Consider encrypting saved design files for sensitive applications

## Benefits of Unified Application

1. **Single Installation**: Deploy one application instead of two
2. **Consistent User Experience**: Unified interface and design language
3. **Easier Maintenance**: Single codebase to maintain and update
4. **Role-Based Access**: Appropriate capabilities for each user type
5. **Shared Components**: Code reuse between Designer and Runtime modes
6. **Simplified Deployment**: One executable to distribute

## Comparison with Original Applications

| Feature | DesignerApp | RuntimeApp | UnifiedApp |
|---------|-------------|------------|------------|
| Design Layouts | ✓ | ✗ | ✓ (Designer role) |
| Save Designs | ✓ | ✗ | ✓ (Designer role) |
| Load Designs | ✓ | ✓ | ✓ (Both roles) |
| Real-time Monitoring | ✗ | ✓ | ✓ (User role) |
| TCP Message Server | ✗ | ✓ | ✓ (User role) |
| Role-Based Access | ✗ | ✗ | ✓ |
| Single Application | ✗ | ✗ | ✓ |

## Troubleshooting

### Login Issues
- Ensure credentials are entered exactly as shown (case-sensitive)
- Check for extra spaces in username/password fields

### Build Errors
- Verify Qt5 development packages are installed
- Check that qmake is in your PATH
- Ensure C++11 support in your compiler

### Runtime Connection Issues
- Verify port 12345 is not blocked by firewall
- Check that no other application is using port 12345
- Ensure external systems are configured to connect to the correct IP address

## License

Part of the Radar System Monitoring Suite.
