# TCP <-> MQTT Bridge (C++20)

Modern C++20 TCP server with MQTT bridge capabilities, using CMake and CPM. Allows bidirectional communication between TCP clients and MQTT brokers.

## Features

- Modern C++20 implementation
- Asynchronous TCP server using Boost.Asio
- Event-driven architecture
- Multi-client support
- Automatic dependency management with CPM
- MQTT Bridge capabilities using async_mqtt
- SLIP protocol support for reliable data framing
- Configuration via YAML
- Comprehensive logging with spdlog

## Dependencies

Core libraries:
- [fmt 11.2.0](https://github.com/fmtlib/fmt) - Modern string formatting
- [spdlog 1.13.0](https://github.com/gabime/spdlog) - Fast C++ logging library
- [yaml-cpp 0.8.0](https://github.com/jbeder/yaml-cpp) - YAML parser and emitter
- [nlohmann/json 3.11.3](https://github.com/nlohmann/json) - JSON for Modern C++
- [Boost.Asio 1.84.0](https://github.com/boostorg/asio) - Networking library
- [async_mqtt 10.1.0](https://github.com/redboltz/async_mqtt) - Asynchronous MQTT client

Required Boost components:
- core
- asio
- system
- assert
- config

## Building

Requirements:
- C++20 compiler
- CMake 3.18+
- Python 3.7+ (for testing)

```bash
git clone <repo>
cd <repo>
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Configuration

Create a `config.yaml` file:

```yaml
tcp:
  port: 12345
  bind: "0.0.0.0"
mqtt:
  broker: "tcp://localhost:1883"
  client_id: "tcp_bridge"
  topics:
    - "device/#"
```

## Usage

### Running the Bridge
```bash
./build/tcp_mqtt_bridge [--config config.yaml]
```

### Testing

1. TCP Test Client (multiple connections):
```bash
./scripts/test_conn.py --clients 10
```

2. MQTT Testing:
```bash
# Subscribe to messages
mosquitto_sub -t "device/#"

# Publish test message
mosquitto_pub -t "device/test" -m "Hello"
```

## Architecture

```
TCP Client <-> TCP Server <-> SLIP Parser <-> MQTT Client <-> MQTT Broker
```

## Project Structure

- `src/`: Source code
  - `main.cpp`: Application entry point
  - `tcp_*.hpp`: TCP server components
  - `mqtt_*.hpp`: MQTT bridge components (planned)
  - `slip_*.hpp`: SLIP protocol handlers (planned)
- `scripts/`: Test utilities
- `config/`: Configuration examples
- `docs/`: Additional documentation