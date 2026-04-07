#!/usr/bin/env python3
"""
MistolitoRPG Start Loop Tool

Sends START_LOOP command to the device to begin game execution without reboot.
Then monitors output until user presses Ctrl+C.

Usage:
    python start_loop.py --port COM8
"""

import serial
import time
import argparse
import threading

BAUDRATE = 115200
TIMEOUT = 5

def main():
    parser = argparse.ArgumentParser(description='MistolitoRPG Start Loop Tool')
    parser.add_argument('--port', required=True, help='Serial port (e.g., COM8)')
    parser.add_argument('--baud', type=int, default=BAUDRATE, help='Baud rate')
    args = parser.parse_args()

    print(f"Connecting to {args.port}...")
    ser = serial.Serial(args.port, args.baud, timeout=TIMEOUT)
    time.sleep(2)
    
    # Send newline to clear any partial command in ESP32 buffer
    ser.write(b'\n')
    time.sleep(0.5)
    
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    time.sleep(0.5)
    
    # Read and discard any pending data
    while ser.in_waiting > 0:
        ser.readline()

    print("Connected!")
    print("Sending START_LOOP command...\n")

    # Send command
    ser.write(b"CMD:START_LOOP\n")
    ser.flush()
    time.sleep(0.5)

    print("="*50)
    print("MONITORING - Press Ctrl+C to exit")
    print("="*50 + "\n")

    try:
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                except:
                    pass
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("\n\nMonitor stopped by user")

    ser.close()
    print("Disconnected.")

if __name__ == "__main__":
    main()
