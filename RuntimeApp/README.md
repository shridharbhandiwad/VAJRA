# Runtime Application

Production application that displays component layouts and receives real-time updates from external systems.

## Features

- Load designs created by Designer Application
- TCP server listening on port 12345
- Real-time component updates (color and size)
- Multi-client support (multiple external systems)
- Comprehensive analytics tracking
- Connection status monitoring

## Building

```bash
cd RuntimeApp
qmake RuntimeApp.pro
make
```

## Running

```bash
./RuntimeApp
```

## Usage

1. **Load Design**: Click "Load Design" to select a .design file
2. **Server Starts**: TCP server automatically starts on port 12345
3. **Wait for Messages**: External systems can now connect and send updates
4. **Monitor Analytics**: Watch the analytics panel for real-time statistics
5. **View Updates**: Components will change color and size as messages arrive

## Server Details

- **Port**: 12345
- **Protocol**: TCP
- **Format**: JSON, newline-delimited
- **Clients**: Supports multiple simultaneous connections

## Message Format

External systems send JSON messages:

```json
{
  "component_id": "component_1",
  "color": "#FF0000",
  "size": 75.5
}
```

## Analytics

The analytics panel shows for each component:
- **ID**: Component identifier
- **Type**: Circle, Square, or Triangle
- **Messages**: Total number of messages received
- **Current Color**: Latest color (hex format)
- **Current Size**: Latest size value
- **Color Changes**: Number of times color changed
- **Size Changes**: Number of times size changed

Plus a total message count across all components.

## Status Bar

The status bar shows:
- Server status (Running/Failed)
- Server port
- Number of connected clients

## Troubleshooting

**Port already in use:**
```bash
# Check what's using port 12345
lsof -i :12345

# Kill the process if needed
kill -9 <PID>
```

**Components not updating:**
- Verify component IDs match between design and external systems
- Check that external systems are connected (see client count)
- Ensure JSON messages are properly formatted

**Design won't load:**
- Verify the .design file is valid JSON
- Check file permissions
- Try creating a new design in Designer Application
