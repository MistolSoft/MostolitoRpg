#!/usr/bin/env python3
"""Wipe pet_data.json from SD card via USB INIT mode"""

import argparse
import serial
import time

def wipe_pet(port: str):
    print(f"Connecting to {port}...")
    ser = serial.Serial(port, 115200, timeout=5)
    time.sleep(0.5)
    
    print("Sending WIPE command (this will also wipe game_tables.json)...")
    ser.write(b"CMD:WIPE:\n")
    time.sleep(0.5)
    
    response = ser.readline().decode().strip()
    print(f"Response: {response}")
    
    print("\nNow you need to:")
    print("1. Run send_init_files.py to restore game_tables.json")
    print("2. Run start_loop.py to start the game (a new pet will be generated)")
    
    ser.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Wipe pet data from SD")
    parser.add_argument("--port", required=True, help="COM port (e.g., COM8)")
    args = parser.parse_args()
    wipe_pet(args.port)
