#!/usr/bin/env python3

import asyncio
import argparse
import sys
import time
import random

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
            # Create and encode message
            message = f"Hello from client {client_id}: {time.time()}".encode()
            slip_packet = slip_encode(message)
            writer.write(slip_packet)
            await writer.drain()
            
            # Read and decode response
            try:
                response = await reader.read(100)
                if response:
                    packets = decoder.decode(response)
                    for packet in packets:
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
