@ -0,0 +1,53 @@
import serial
import threading
import sys

# KONFIGURATION
REAL_PORT = 'COM3'       # Dein ESP32
VIRTUAL_PORT = 'COM10'   # Eine Seite des com0com Paars
BAUD_RATE = 38200       # Muss mit ESP32 übereinstimmen

def bridge(source, destination, label, color_code):
    """Liest Daten von source und schreibt sie nach destination."""
    while True:
        try:
            if source.in_waiting > 0:
                data = source.read(source.in_waiting)
                destination.write(data)
                
                # Ausgabe in der Konsole
                # Nutzt ANSI Colors für bessere Lesbarkeit (PC -> ESP vs ESP -> PC)
                print(f"{color_code}[{label}]: {data.hex(' ')} | {data}{'\033[0m'}")
        except Exception as e:
            print(f"Fehler in {label}: {e}")
            break

def start_sniffer():
    try:
        # Ports öffnen
        ser_real = serial.Serial(REAL_PORT, BAUD_RATE, timeout=0.1)
        ser_virt = serial.Serial(VIRTUAL_PORT, BAUD_RATE, timeout=0.1)
        
        print(f"--- Sniffer aktiv ---")
        print(f"Real: {REAL_PORT} <--> Virtual: {VIRTUAL_PORT}")
        print(f"Verbinde deine Windows-App jetzt mit dem ANDEREN Ende (z.B. COM11)")
        print("-" * 50)

        # Threads für beide Richtungen starten
        # Blau: Daten vom ESP32 zum PC
        t1 = threading.Thread(target=bridge, args=(ser_real, ser_virt, "ESP -> PC", "\033[94m"))
        # Grün: Daten vom PC zum ESP32
        t2 = threading.Thread(target=bridge, args=(ser_virt, ser_real, "PC -> ESP", "\033[92m"))

        t1.start()
        t2.start()

        t1.join()
        t2.join()

    except serial.SerialException as e:
        print(f"Konnte Ports nicht öffnen: {e}")
        print("Stelle sicher, dass kein anderes Programm COM3 oder COM10 belegt.")

if __name__ == "__main__":
    start_sniffer()