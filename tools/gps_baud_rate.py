#!/usr/bin/env python3
"""
GPS Baud Rate Changer & Data Reader
Untuk modul GPS u-blox (GT-U7, NEO-6M, dll)
"""
import serial
import sys
import time
import struct

def send_ubx(serial_conn, msg_class, msg_id, payload):
    """Kirim UBX binary message"""
    length = len(payload)
    header = struct.pack('<BBBHH', 0xB5, 0x62, msg_class, msg_id, length)
    checksum = 0
    for b in header[2:]:
        checksum ^= b
    for b in payload:
        checksum ^= b
    checksum_bytes = struct.pack('<BB', checksum >> 8, checksum & 0xFF)
    serial_conn.write(header + payload + checksum_bytes)

def nmea_checksum(sentence):
    """Hitung checksum NMEA (XOR semua karakter antara $ dan *)"""
    checksum = 0
    for c in sentence:
        checksum ^= ord(c)
    return checksum

def change_baud_rate(port, old_baud, new_baud):
    """Ubah baud rate GPS module pakai PMTK command (lebih compatible)"""
    print(f"Mengubah baud rate dari {old_baud} ke {new_baud}...")

    ser = serial.Serial(port, old_baud, timeout=2)
    time.sleep(0.1)

    # PMTK251: ubah baud rate
    cmd_body = f"PMTK251,{new_baud}"
    checksum = nmea_checksum(cmd_body)
    pmtk_cmd = f"${cmd_body}*{checksum:02X}\r\n"
    print(f"  TX: {pmtk_cmd.strip()}")
    ser.write(pmtk_cmd.encode('ascii'))
    time.sleep(0.5)

    # PMTK250: set update rate 1Hz (default)
    cmd_body2 = "PMTK250,1,1,0,0,0,0"
    checksum2 = nmea_checksum(cmd_body2)
    pmtk_rate = f"${cmd_body2}*{checksum2:02X}\r\n"
    print(f"  TX: {pmtk_rate.strip()}")
    ser.write(pmtk_rate.encode('ascii'))
    time.sleep(0.5)

    # PMTK314: set NMEA output (GPRMC + GPGGA only)
    cmd_body3 = "PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"
    checksum3 = nmea_checksum(cmd_body3)
    pmtk_out = f"${cmd_body3}*{checksum3:02X}\r\n"
    print(f"  TX: {pmtk_out.strip()}")
    ser.write(pmtk_out.encode('ascii'))
    time.sleep(0.5)

    # PMTK605: save ke flash
    cmd_body4 = "PMTK605"
    checksum4 = nmea_checksum(cmd_body4)
    pmtk_save = f"${cmd_body4}*{checksum4:02X}\r\n"
    print(f"  TX: {pmtk_save.strip()}")
    ser.write(pmtk_save.encode('ascii'))
    time.sleep(1.0)

    ser.close()
    print(f"\nBaud rate diubah ke {new_baud}")
    print("GPS module akan restart otomatis...")

def read_gps_data(port, baud):
    """Baca dan tampilkan data NMEA dari GPS"""
    print(f"\nMembaca data GPS dari {port} @ {baud} baud...")
    print("Tekan Ctrl+C untuk berhenti\n")

    ser = serial.Serial(port, baud, timeout=1)
    last_print = 0

    # GPS state
    gps = {
        'time': '??:??:??', 'date': '?/?/??', 'status': '?',
        'lat': 0.0, 'lon': 0.0, 'speed': 0.0, 'course': 0.0,
        'fix': 'N', 'sats': '?', 'alt': '?', 'hdop': '?'
    }

    try:
        while True:
            line = ser.readline().decode('ascii', errors='ignore').strip()

            if not line.startswith('$'):
                continue

            parts = line.split('*')[0].split(',') if '*' in line else line.split(',')
            if not parts[0].startswith('$'):
                continue

            # GPRMC
            if parts[0] == '$GPRMC' and len(parts) >= 8:
                gps['status'] = parts[2] if parts[2] else '?'
                t = parts[1] if len(parts) > 1 and parts[1] else ''
                if len(t) >= 6:
                    gps['time'] = f"{t[0:2]}:{t[2:4]}:{t[4:6]}"
                d = parts[9] if len(parts) > 9 and parts[9] else ''
                if len(d) >= 6:
                    gps['date'] = f"{d[0:2]}/{d[2:4]}/20{d[4:6]}"

                if gps['status'] == 'A' and parts[3] and parts[5]:
                    lat_raw = parts[3]
                    lat_dir = parts[4]
                    lat_deg = float(lat_raw[:2])
                    lat_min = float(lat_raw[2:])
                    gps['lat'] = (lat_deg + lat_min / 60.0) * (-1 if lat_dir == 'S' else 1)

                    lon_raw = parts[5]
                    lon_dir = parts[6]
                    lon_deg = float(lon_raw[:3])
                    lon_min = float(lon_raw[3:])
                    gps['lon'] = (lon_deg + lon_min / 60.0) * (-1 if lon_dir == 'W' else 1)

                    gps['speed'] = float(parts[7]) * 1.852 if parts[7] else 0.0
                    gps['course'] = float(parts[8]) if parts[8] and parts[8] != '' else 0.0

            # GPGGA
            elif parts[0] == '$GPGGA' and len(parts) >= 10:
                gps['fix'] = 'Y' if parts[6] and int(parts[6]) > 0 else 'N'
                gps['sats'] = parts[7] if parts[7] else '?'
                gps['hdop'] = parts[8] if parts[8] else '?'
                gps['alt'] = parts[9] if parts[9] else '?'

            now = time.time()
            if now - last_print >= 0.05 and gps['status'] == 'A':
                last_print = now
                print(f"  {gps['time']} | {gps['date']} | Fix: {gps['fix']} | Sats: {gps['sats']:>2} | HDOP: {gps['hdop']}")
                print(f"  Lat: {gps['lat']:.6f} | Lon: {gps['lon']:.6f} | Alt: {gps['alt']}m")
                print(f"  Speed: {gps['speed']:.1f} km/h | Course: {gps['course']:.1f} deg")
                print()

    except KeyboardInterrupt:
        print("\nSelesai.")
    finally:
        ser.close()

def list_ports():
    """List serial ports yang tersedia"""
    import glob
    ports = glob.glob('/dev/cu.*') + glob.glob('/dev/tty.*')
    ports = [p for p in ports if 'Bluetooth' not in p and 'debug' not in p and 'wlan' not in p]
    return sorted(ports)

def auto_detect_gps(baud_list=[9600, 115200]):
    """Auto-detect port dan baud rate GPS"""
    ports = list_ports()
    if not ports:
        print("Tidak ada serial port ditemukan!")
        return None, None

    print(f"\nMencari GPS di {len(ports)} port...")
    for port in ports:
        for baud in baud_list:
            try:
                ser = serial.Serial(port, baud, timeout=2)
                time.sleep(1.5)
                for _ in range(10):
                    line = ser.readline().decode('ascii', errors='ignore').strip()
                    if line.startswith('$GP') or line.startswith('$GN'):
                        ser.close()
                        print(f"GPS ditemukan di {port} @ {baud} baud!")
                        return port, baud
                ser.close()
            except:
                pass
    print("GPS tidak terdeteksi di semua port!")
    return None, None

def main():
    print("=" * 50)
    print("GPS Baud Rate Changer & Data Reader")
    print("=" * 50)

    ports = list_ports()
    print("\nSerial ports tersedia:")
    for i, p in enumerate(ports, 1):
        print(f"  {i}. {p}")

    print("\nPilihan:")
    print("  0. Auto-detect GPS")
    print(f"  1-{len(ports)}. Pilih port manual")

    choice = input("\nPilih: ").strip()

    port = None
    baud = None

    if choice == '0' or choice == '':
        port, baud = auto_detect_gps()
        if not port:
            return
    else:
        try:
            idx = int(choice) - 1
            port = ports[idx]
        except:
            print("Pilihan tidak valid!")
            return

    print(f"\nPort: {port}")
    print(f"Baud: {baud or '?'}")
    print("\nPilihan:")
    print("  1. Ubah baud rate (9600 -> 115200)")
    print("  2. Ubah baud rate (115200 -> 9600)")
    print("  3. Baca data GPS")
    print("  0. Keluar")

    choice = input("\nPilih (0-3): ").strip()

    if choice == '1':
        change_baud_rate(port, 9600, 115200)
        input("\nTekan Enter untuk baca data GPS...")
        read_gps_data(port, 115200)

    elif choice == '2':
        change_baud_rate(port, 115200, 9600)
        input("\nTekan Enter untuk baca data GPS...")
        read_gps_data(port, 9600)

    elif choice == '3':
        # Coba beberapa baud rate
        print(f"\nMencoba baca GPS dari {port} di beberapa baud rate...")
        for test_baud in [9600, 4800, 38400, 57600, 115200]:
            print(f"\n  Mencoba {test_baud} baud...", end=' ')
            try:
                ser = serial.Serial(port, test_baud, timeout=2)
                time.sleep(1.5)
                found = False
                for _ in range(10):
                    line = ser.readline().decode('ascii', errors='ignore').strip()
                    if line.startswith('$GP') or line.startswith('$GN'):
                        print(f"✅ GPS ditemukan di {test_baud} baud!")
                        ser.close()
                        read_gps_data(port, test_baud)
                        return
                ser.close()
                print("❌ tidak ada data")
            except:
                print("❌ error")
        print("\nGPS tidak terdeteksi di semua baud rate!")

    elif choice == '0':
        return
    else:
        print("Pilihan tidak valid!")

if __name__ == '__main__':
    main()
