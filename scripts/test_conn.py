#!/usr/bin/env python3

import asyncio
import argparse
import sys
import time
import random
import struct

# SLIP special characters
SLIP_END = 0xC0
SLIP_ESC = 0xDB
SLIP_ESC_END = 0xDC
SLIP_ESC_ESC = 0xDD

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
        
    def decode(self, data: bytes) -> list[bytes]:
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
                        self.buffer.clear()
                elif byte == SLIP_ESC:
                    self.escaped = True
                else:
                    self.buffer.append(byte)
        return packets

async def client_connection(host: str, port: int, client_id: int):
    writer = None
    decoder = SlipDecoder()
    try:
        reader, writer = await asyncio.open_connection(host, port)
        print(f"Client {client_id}: Connected")
        
        while True:
            # Create sensor data packet
            # Format:
            # - packet_type: uint8 = 0x10
            # - sensor_id: uint16
            # - temperature: float32
            # - humidity: float32
            # - pressure: float32
            packet = bytearray()
            packet.append(0x10)  # packet_type for sensor_data
            packet.extend(client_id.to_bytes(2, 'little'))  # sensor_id as uint16
            # Generate some simulated sensor data
            temperature = 20.0 + random.uniform(-5, 5)  # temperature around 20°C
            humidity = 50.0 + random.uniform(-10, 10)  # humidity around 50%
            pressure = 1013.25 + random.uniform(-10, 10)  # pressure around 1 atm
            # Pack floats in little-endian
            packet.extend(struct.pack('<f', temperature))
            packet.extend(struct.pack('<f', humidity))
            packet.extend(struct.pack('<f', pressure))
            
            slip_packet = slip_encode(packet)
            writer.write(slip_packet)
            await writer.drain()
            
            # Read and decode response
            try:
                response = await reader.read(100)
                if response:
                    packets = decoder.decode(response)
                    for packet in packets:
                        if len(packet) == 1 and packet[0] == 0x06:  # ACK
                            print(f"Client {client_id}: Received ACK")
                        else:
                            print(f"Client {client_id}: Decoded response: {packet.hex()} ({packet[0]:02x})")
            except Exception as e:
                print(f"Client {client_id}: Error reading response: {e}")
            
            await asyncio.sleep(random.uniform(0.5, 2.0))
            
    except ConnectionRefusedError:
        print(f"Client {client_id}: Connection refused")
    except Exception as e:
        print(f"Client {client_id}: Error: {e}")
    finally:
        if writer is not None:
            writer.close()
            await writer.wait_closed()
            print(f"Client {client_id}: Disconnected")

async def main():
    parser = argparse.ArgumentParser(description='TCP connection tester')
    parser.add_argument('-H', '--host', default='localhost',
                      help='Server host (default: localhost)')
    parser.add_argument('-p', '--port', type=int, default=12345,
                      help='Server port (default: 12345)')
    parser.add_argument('-n', '--clients', type=int, default=5,
                      help='Number of clients (default: 5)')
    args = parser.parse_args()

    print(f"Starting {args.clients} clients connecting to {args.host}:{args.port}")
    
    # Create multiple client connections
    tasks = [
        client_connection(args.host, args.port, i)
        for i in range(args.clients)
    ]
    
    try:
        await asyncio.gather(*tasks)
    except KeyboardInterrupt:
        print("\nShutting down...")
        sys.exit(0)

if __name__ == "__main__":
    asyncio.run(main())
