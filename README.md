# Wi-Fi Shield (ESP32)

Firmware for a custom ESP32-based Wi-Fi shield used in my bachelor's thesis project.

The firmware:
- starts an ESP32 SoftAP
- listens for UDP packets on a fixed port
- forwards received UDP payload bytes to the main board over UART
- renders scrolling status information on a 128x32 OLED display

## Hardware

- Arduino Nano ESP32 (ESP32-S3)
- Waveshare SSD1306 128x32 OLED (I2C)
- Main board connected via UART

### Pin Usage

- UART RX (`Serial1`): GPIO `4`
- UART TX (`Serial1`): GPIO `5`
- OLED I2C: uses board default `SDA` and `SCL`

## Build and Flash

### Arduino IDE

1. Install Arduino Nano ESP32 Boards Manager.
2. Install required libraries:
   - `U8g2`
   - `AsyncUDP` (included with ESP32 core)
3. Open `wifi-shield/wifi-shield.ino`.
4. Select your board and serial port.
5. Build and upload. (It might be necessary to use the "Upload Using Programmer" option if the USB serial connection is not working.)

## Runtime Behavior

On boot, the firmware:

1. Initializes USB serial and UART.
2. Configures and starts a SoftAP.
3. Starts a UDP listener on specified port.
4. Prints AP and packet diagnostics to USB serial.
5. For each UDP packet:
   - logs packet metadata to USB serial
   - forwards raw packet payload to `Serial1`
6. Updates OLED display with scrolling SSID/password/IP/port lines.

## Notes and Limitations

- `strToCharArr()` allocates memory dynamically and does not free it. In current usage this happens once during setup, so the impact is limited.
- The AP credentials are hardcoded in source.

## License

Source files include SPDX header `Apache-2.0`.
