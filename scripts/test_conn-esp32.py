import network
import socket
import time
import random
import struct
import json
import os

# SLIP special characters
SLIP_END = 0xC0
SLIP_ESC = 0xDB
SLIP_ESC_END = 0xDC
SLIP_ESC_ESC = 0xDD

# Default configuration - will be overridden by config file
CONFIG = {
    'wifi_ssid': 'your_ssid',
    'wifi_password': 'your_password',
    'server_host': 'localhost',
    'server_port': 12345,
    'sensor_id': 1
}

def load_config():
    try:
        with open('config.json', 'r') as f:
            config = json.load(f)
            CONFIG.update(config)
    except:
        # # If config file doesn't exist, create it with default values
        # with open('config.json', 'w') as f:
        #     json.dump(CONFIG, f, indent=2)
        # print("Created default config.json. Please edit with your settings.")
        return False
    return True

def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        print('Connecting to WiFi...')
        wlan.connect(CONFIG['wifi_ssid'], CONFIG['wifi_password'])
        retry = 10
        while not wlan.isconnected() and retry > 0:
            time.sleep(1)
            retry -= 1
    if wlan.isconnected():
        print('WiFi connected:', wlan.ifconfig()[0])
        return True
    print('WiFi connection failed!')
    return False

def slip_encode(data: bytes) -> bytes:
    result = bytearray([SLIP_END])
    for byte in data:
        if byte == SLIP_END:
            result.extend([SLIP_ESC, SLIP_ESC_END])
        elif byte == SLIP_ESC:
            result.extend([SLIP_ESC, SLIP_ESC_ESC])
        else:
            result.append(byte)
    result.append(SLIP_END)
    return bytes(result)

class SlipDecoder:
    def __init__(self):
        self.buffer = bytearray()
        self.escaped = False
        
    def decode(self, data: bytes) -> list:
        packets = []
        for byte in data:
            if self.escaped:
                if byte == SLIP_ESC_END:
                    self.buffer.append(SLIP_END)
                elif byte == SLIP_ESC_ESC:
                    self.buffer.append(SLIP_ESC)
                else:
                    print(f"Invalid escape sequence: {byte:02x}")
                self.escaped = False
            else:
                if byte == SLIP_END:
                    if self.buffer:
                        packets.append(bytes(self.buffer))
                        self.buffer = bytearray()  # Create new empty buffer instead of clear()
                elif byte == SLIP_ESC:
                    self.escaped = True
                else:
                    self.buffer.append(byte)
        return packets

def send_sensor_data(sock, decoder, sensor_id):
    # Create sensor data packet
    # Format:
    # - packet_type: uint8 = 0x10
    # - sensor_id: uint16
    # - temperature: float32
    # - humidity: float32
    # - pressure: float32
    packet = bytearray()
    packet.append(0x10)  # packet_type for sensor_data
    packet.extend(sensor_id.to_bytes(2, 'little'))  # sensor_id as uint16
    
    # Generate some simulated sensor data
    temperature = 20.0 + random.uniform(-5, 5)  # temperature around 20°C
    humidity = 50.0 + random.uniform(-10, 10)  # humidity around 50%
    pressure = 1013.25 + random.uniform(-10, 10)  # pressure around 1 atm
    
    # Pack floats in little-endian
    packet.extend(struct.pack('<f', temperature))
    packet.extend(struct.pack('<f', humidity))
    packet.extend(struct.pack('<f', pressure))
    
    slip_packet = slip_encode(packet)
    sock.send(slip_packet)
    
    # Read and decode response
    try:
        response = sock.recv(100)
        if response:
            packets = decoder.decode(response)
            for packet in packets:
                if len(packet) == 1 and packet[0] == 0x06:  # ACK
                    print(f"Sensor {sensor_id}: Received ACK")
                    print(f"Values - Temp: {temperature:.1f}°C, Humidity: {humidity:.1f}%, Pressure: {pressure:.1f}hPa")
                else:
                    print(f"Sensor {sensor_id}: Decoded response: {packet.hex()}")
    except Exception as e:
        print(f"Error reading response: {e}")
        raise e

def main():
    if not load_config():
        return
    
    if not connect_wifi():
        return
    
    decoder = SlipDecoder()
    sock = None
    
    while True:
        try:
            if sock is None:
                print(f"Connecting to {CONFIG['server_host']}:{CONFIG['server_port']}...")
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.connect((CONFIG['server_host'], CONFIG['server_port']))
                print("Connected to server")
            
            send_sensor_data(sock, decoder, CONFIG['sensor_id'])
            time.sleep(random.uniform(0.5, 2.0))
            
        except Exception as e:
            print(f"Connection error: {e}")
            if sock:
                sock.close()
                sock = None
            time.sleep(5)  # Wait before retrying

if __name__ == "__main__":
    main()