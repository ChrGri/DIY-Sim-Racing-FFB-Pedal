import hid
import time
import struct

def list_hid_devices():
    """List all connected HID devices."""
    for device in hid.enumerate():
        print(f"Vendor ID: {device['vendor_id']:04x}, "
              f"Product ID: {device['product_id']:04x}, "
              f"Manufacturer: {device['manufacturer_string']}, "
              f"Product: {device['product_string']}")

def read_game_controller_reports(vendor_id, product_id, vendor_id_2, product_id_2, output_file):
    """Read input reports from the HID game controller."""
    try:
        # Open the HID device
        device = hid.Device(vendor_id, product_id)
        device_2 = hid.Device(vendor_id_2, product_id_2)

        # Open the output file
        with open(output_file, 'w') as file:
            file.write("Time in ms, Report Data, Report Data Bridge\n")  # Header row for the data
            timestamp_initial = time.time()

            # Continuously read data from the device
            print("Reading input reports. Press Ctrl+C to stop.")
            while True:
                report = device.read(16)  # Adjust the size as per your device's report size
                report_2 = device_2.read(16)  # Adjust the size as per your device's report size


                timestamp = (time.time() - timestamp_initial) * 1000  # Current time in seconds (float)
                if report:

                    header = report[0]  # First byte
                    data = struct.unpack('<H', report[1:3])[0]  # Next 2 bytes as a 16-bit integer

                    # bridge data
                    data_2 = struct.unpack('<H', report_2[7:9])[0]  # Next 2 bytes as a 16-bit integer

                    # Format the data as a CSV-style row
                    data_row = f"{timestamp:.6f}, {data:.6f}, {data_2:.6f}\n"

                    # Write the row to the file
                    file.write(data_row)



    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'device' in locals():
            device.close()

if __name__ == "__main__":
    # List devices to get vendor and product IDs
    print("Available HID devices:")
    list_hid_devices()

    # Replace with your game controller's Vendor ID and Product ID
    # brake V4
    VENDOR_ID = 0x3035  # Example Vendor ID
    PRODUCT_ID = 0x8215  # Example Product ID

    # bridge
    VENDOR_ID_2 = 0x3035  # Example Vendor ID
    PRODUCT_ID_2 = 0x8213  # Example Product ID

    OUTPUT_FILE = "gamepad_data.txt"  # Path to the output file

    # Read game controller reports
    read_game_controller_reports(VENDOR_ID, PRODUCT_ID, VENDOR_ID_2, PRODUCT_ID_2, OUTPUT_FILE)
