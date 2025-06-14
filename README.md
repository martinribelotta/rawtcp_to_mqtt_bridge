# TCP <-> MQTT Bridge

A high-performance bridge between TCP devices and MQTT brokers, with flexible packet parsing and templating capabilities.

## Overview

This project provides a robust bridge between TCP-connected devices and MQTT brokers. It features:

- **Flexible Packet Parsing**: Define your packet structures in YAML files with support for:
  - Multiple data types (integers, floats, arrays)
  - Bitfields for flag handling
  - Fixed and variable length fields
  - Automatic packet identification

- **Smart MQTT Integration**:
  - Per-packet MQTT topic and payload templates
  - JSON payload formatting
  - Template-based message transformation
  - Dynamic topic generation based on packet content

- **Reliable Communications**:
  - SLIP framing protocol for reliable packet delimitation
  - Asynchronous I/O for high performance
  - Multi-client support
  - Automatic reconnection handling

## Dependencies

The project is built with modern C++20 and uses:

### Core Components

- [Boost 1.84.0+](https://www.boost.org/) - Core networking (Asio)
- [async_mqtt](https://github.com/redboltz/async_mqtt) - MQTT client
- [inja](https://github.com/pantor/inja) - Template engine
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) - YAML parsing
- [nlohmann/json](https://github.com/nlohmann/json) - JSON handling
- [fmt](https://github.com/fmtlib/fmt) & [spdlog](https://github.com/gabime/spdlog) - Logging

## Configuration

The bridge uses two types of configuration files:

### Main Configuration (`config.yaml`)

```yaml
tcp:
  port: 12345
  bind: "0.0.0.0"
mqtt:
  host: "localhost"
  port: 1883
  client_id: "tcp_bridge"
logging:
  level: "debug"
packet_defs:
  paths:
    - "packets"           # Relative to config.yaml
    - "/etc/tcp_bridge/packets"  # Absolute path
  patterns:
    - "*.yaml"
    - "*.yml"
```

### Packet Definitions

Packet structures are defined in YAML files that can be organized in directories. Example:

```yaml
sensor_data:
  mqtt:
    topic: "sensors/data_sensor_{{sensor_id}}"
    payload: |
      {
        "temperature": {{temperature}},
        "humidity": {{humidity}},
        "pressure": {{pressure}}
      }
  fields:
    - name: packet_type
      type: uint8
      offset: 0
      value: 0x10  # Identifier

    - name: sensor_id
      type: uint16
      offset: 1

    - name: temperature
      type: float32
      offset: 3
```

## Building & Running

Requirements:

- C++20 compiler (GCC 10+, Clang 10+)
- CMake 3.18+
- Boost 1.84.0+

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/tcp_mqtt_bridge -c config.yaml
```

Command line options:

- `-c, --config`: Configuration file path
- `-p, --port`: TCP port (overrides config)
- `-b, --bind`: Bind address (overrides config)
- `-l, --log-level`: Set log level
- `-v, --verbose`: Enable debug logging
- `-h, --help`: Show help

## Project Structure

```plaintext
├── config/
│   ├── config.yaml          # Main configuration
│   └── packets/             # Packet definitions
│       ├── sensors/         # Organized by type
│       ├── commands.yaml
│       └── status.yaml
├── src/
│   ├── main.cpp            # Entry point
│   ├── packet_*.{hpp,cpp}  # Packet processing
│   ├── mqtt_*.{hpp,cpp}    # MQTT client
│   └── tcp_*.{hpp,cpp}     # TCP server
└── scripts/
    └── test_conn.py        # Testing utilities
```

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.
