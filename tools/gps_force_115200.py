#!/usr/bin/env python3
"""Force GPS ke 115200 — tidak tutup port"""
import serial
import time
import glob

def nmea_checksum(sentence):
    checksum = 0
    for c in sentence:
        checksum ^= ord(c)
    return checksum

def find_gps_port():
    for pattern in ['/dev/cu.SLAB_USBtoUART', '/dev/cu.usbserial-*']:
        for port in glob.glob(pattern):
            try:
                ser = serial.Serial(port, 9600, timeout=2)
                time.sleep(1.5)
                for _ in range(5):
                    line = ser.readline().decode('ascii', errors='ignore').strip()
                    if line.startswith('$GP') or line.startswith('$GN'):
                        ser.close()
                        return port
                ser.close()
            except:
                pass
    return None

print("Mencari GPS...")
port = find_gps_port()
if not port:
    print("GPS tidak ditemukan!")
    exit(1)
print(f"GPS ditemukan di {port}")

print("\nKirim PMTK251 untuk ubah ke 115200...")
ser = serial.Serial(port, 9600, timeout=2)
time.sleep(0.5)

# Kirim PMTK251,115200
cmd = "PMTK251,115200"
cs = nmea_checksum(cmd)
ser.write(f"${cmd}*{cs:02X}\r\n".encode('ascii'))
print(f"  TX: $PMTK251,115200*{cs:02X}")
time.sleep(0.3)

# Kirim PMTK605 (save)
cmd2 = "PMTK605"
cs2 = nmea_checksum(cmd2)
ser.write(f"${cmd2}*{cs2:02X}\r\n".encode('ascii'))
print(f"  TX: $PMTK605*{cs2:02X}")
time.sleep(0.5)

ser.close()
print("\nPort ditutup. Tunggu GPS restart...")
time.sleep(3.0)

# Coba detek di 115200 dulu, lalu 9600
for test_baud in [115200, 9600]:
    print(f"\nCoba baca di {test_baud} baud...")
    try:
        ser = serial.Serial(port, test_baud, timeout=3)
        time.sleep(2.0)
        found = False
        for i in range(10):
            line = ser.readline().decode('ascii', errors='ignore').strip()
            if line.startswith('$GP') or line.startswith('$GN'):
                print(f"  OK! GPS aktif di {test_baud} baud!")
                ser.close()
                exit(0)
        ser.close()
    except Exception as e:
        print(f"  Error: {e}")

print("\nGPS tetap di 9600. GT-U7 clone tidak support save baud rate.")
print("Pakai 9600 baud saja atau gunakan u-center di Windows.")
