#!/usr/bin/env python3
"""
MistolitoRPG USB Init Tool

This script sends initialization files to MistolitoRPG device via USB Serial.

Usage:
    python send_init_files.py --port COM3
    python send_init_files.py --port COM3 --wipe
"""

import serial
import base64
import os
import sys
import time
import argparse

CHUNK_SIZE = 128
BAUDRATE = 115200
TIMEOUT = 5


class MistolitoUSBInit:
    def __init__(self, port, baudrate=BAUDRATE):
        self.port = port
        self.baudrate = baudrate
        self.ser = None

    def connect(self):
        print(f"Connecting to {self.port}...")
        self.ser = serial.Serial(self.port, self.baudrate, timeout=TIMEOUT)
        time.sleep(2)

        self.ser.write(b'\n')
        time.sleep(0.2)

        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        time.sleep(0.5)

        while self.ser.in_waiting > 0:
            self.ser.readline()

        print("Connected!")
        return True

    def disconnect(self):
        if self.ser:
            self.ser.close()
            print("Disconnected.")

    def send_command(self, cmd, wait_for_response=True):
        full_cmd = f"CMD:{cmd}\n"
        self.ser.write(full_cmd.encode('utf-8'))
        self.ser.flush()

        time.sleep(0.5)

        if not wait_for_response:
            return ""

        for _ in range(30):
            if self.ser.in_waiting > 0:
                response = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if response:
                    if response.startswith("FILE_") or response.startswith("ERROR:") or response.startswith("STATUS:") or response.startswith("INIT_") or response.startswith("WIPE_") or response.startswith("START_"):
                        return response
            else:
                time.sleep(0.1)

        return ""

    def send_file(self, filepath, remote_path=None):
        filename = os.path.basename(filepath)
        filepath = os.path.abspath(filepath)

        if not os.path.exists(filepath):
            print(f"Error: File not found: {filepath}")
            return False

        filesize = os.path.getsize(filepath)

        if remote_path:
            remote_file_path = remote_path
        else:
            remote_file_path = f"/DATA/{filename}"

        print(f"Sending {filename} ({filesize} bytes)...")

        response = self.send_command(f"FILE_START:{remote_file_path}:{filesize}")
        print(f" Response: {response}")

        if not response.startswith("FILE_RECV_START"):
            print("FAILED")
            return False

        with open(filepath, 'rb') as f:
            sent = 0
            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break

                encoded = base64.b64encode(chunk).decode('utf-8')
                sent += len(chunk)

                pct = int((sent / filesize) * 100)
                print(f"\r {pct}%", end="", flush=True)

                response = self.send_command(f"FILE_DATA:{encoded}")

                if "ERROR" in response:
                    print(" FAILED")
                    return False

        print()

        response = self.send_command("FILE_END")

        if "FILE_RECV_OK" in response:
            print(" OK")
            return True
        else:
            print(" FAILED")
            return False

    def check_status(self):
        response = self.send_command("STATUS")
        print(f"Status: {response}")
        return response

    def delete_file(self, remote_path):
        print(f"Deleting {remote_path}...", end=" ", flush=True)
        response = self.send_command(f"DELETE:{remote_path}")
        
        if "DELETE_OK" in response or "FILE_NOT_FOUND" in response:
            print("OK")
            return True
        else:
            print(f"Response: {response}")
            return False

    def complete_init(self):
        print("Completing initialization...", end=" ", flush=True)
        response = self.send_command("INIT_COMPLETE")

        if "INIT_COMPLETE" in response:
            print("OK")
            print("\n" + "="*50)
            print("Initialization complete!")
            print("The device will reboot.")
            print("="*50)
            return True
        else:
            print("FAILED")
            return False

    def wipe(self):
        print("Wiping all data...", end=" ", flush=True)
        response = self.send_command("WIPE")

        if "WIPE_COMPLETE" in response:
            print("OK")
            print("Device wiped. Rebooting...")
            return True
        print("FAILED")
        return False


def main():
    parser = argparse.ArgumentParser(description='MistolitoRPG USB Init Tool')
    parser.add_argument('--port', required=True, help='Serial port (e.g., COM3 or /dev/ttyUSB0)')
    parser.add_argument('--tables-dir', default='firmware/data/tables',
                        help='Directory containing table JSON files')
    parser.add_argument('--dna-dir', default='firmware/data/dna',
                        help='Directory containing DNA JSON file')
    parser.add_argument('--pet-data', default='firmware/data/pet_data.json',
                        help='Path to pet_data.json')
    parser.add_argument('--baud', type=int, default=BAUDRATE, help='Baud rate')
    parser.add_argument('--wipe', action='store_true', help='Wipe device and reboot')
    parser.add_argument('--status', action='store_true', help='Check device status only')

    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    init = MistolitoUSBInit(args.port, args.baud)

    try:
        init.connect()

        if args.status:
            init.check_status()
            return

        if args.wipe:
            init.wipe()
            return

        tables_dir = args.tables_dir
        if not os.path.isabs(tables_dir):
            tables_dir = os.path.join(project_root, tables_dir)

        table_files = ['professions.json', 'enemies.json', 'config.json']

        for table_file in table_files:
            table_path = os.path.join(tables_dir, table_file)
            if os.path.exists(table_path):
                if not init.send_file(table_path, f"/DATA/TABLES/{table_file}"):
                    print(f"Failed to send {table_file}")
                    return
            else:
                print(f"Warning: {table_file} not found")

        dna_dir = args.dna_dir
        if not os.path.isabs(dna_dir):
            dna_dir = os.path.join(project_root, dna_dir)

        dna_file = os.path.join(dna_dir, 'pet_dna.json')
        if os.path.exists(dna_file):
            if not init.send_file(dna_file, "/DATA/DNA/pet_dna.json"):
                print("Failed to send pet_dna.json")
                return
        else:
            print("Warning: pet_dna.json not found")

        # Delete pet_data.json to force regeneration from DNA on first boot
        print("Deleting pet_data.json to force DNA-based initialization...")
        init.delete_file("/BRAIN/PET/pet_data.json")

        if not init.complete_init():
            return

    except serial.SerialException as e:
        print(f"Serial error: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nCancelled by user")
    finally:
        init.disconnect()


if __name__ == "__main__":
    main()
